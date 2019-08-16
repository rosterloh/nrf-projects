#include <zephyr.h>
#include <sensor.h>
#include <device.h>
#include <i2c.h>
#include <gpio.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(seesaw, CONFIG_SEESAW_LOG_LEVEL);

#define SEESAW_I2C_DEV_NAME		DT_INST_0_ADAFRUIT_SEESAW_BUS_NAME

#define SEESAW_IRQ_GPIO_DEV_NAME	DT_INST_0_ADAFRUIT_SEESAW_IRQ_GPIOS_CONTROLLER
#define SEESAW_IRQ_GPIO_PIN		DT_INST_0_ADAFRUIT_SEESAW_IRQ_GPIOS_PIN

enum async_init_step {
	ASYNC_INIT_STEP_VERIFY_ID,
	ASYNC_INIT_STEP_CONFIGURE,

	ASYNC_INIT_STEP_COUNT
};

struct seesaw_data {
	struct device             *irq_gpio_dev;
	struct device             *i2c_dev;
	struct gpio_callback      irq_gpio_cb;
	struct k_spinlock         lock;
	sensor_trigger_handler_t  data_ready_handler;
	struct k_work		  trigger_handler_work;
	struct k_delayed_work     init_work;
	enum async_init_step      async_init_step;
	int                       err;
	bool                      ready;
};

static const s32_t async_init_delay[ASYNC_INIT_STEP_COUNT] = {
	[ASYNC_INIT_STEP_VERIFY_ID] = 1,
	[ASYNC_INIT_STEP_CONFIGURE] = 0,
};

static int seesaw_async_init_verify_id(struct seesaw_data *dev_data);
static int seesaw_async_init_configure(struct seesaw_data *dev_data);

static int (* const async_init_fn[ASYNC_INIT_STEP_COUNT])(struct seesaw_data *dev_data) = {
	[ASYNC_INIT_STEP_VERIFY_ID] = seesaw_async_init_verify_id,
	[ASYNC_INIT_STEP_CONFIGURE] = seesaw_async_init_configure,
};

static struct seesaw_data seesaw_data;
DEVICE_DECLARE(seesaw);

static void irq_handler(struct device *gpiob, struct gpio_callback *cb,
			u32_t pins)
{
	int err;

	err = gpio_pin_disable_callback(seesaw_data.irq_gpio_dev,
					SEESAW_IRQ_GPIO_PIN);
	if (unlikely(err)) {
		LOG_ERR("Cannot disable IRQ");
		k_panic();
	}

	k_work_submit(&seesaw_data.trigger_handler_work);
}

static void trigger_handler(struct k_work *work)
{
	sensor_trigger_handler_t handler;
	int err = 0;

	k_spinlock_key_t key = k_spin_lock(&seesaw_data.lock);
	handler = seesaw_data.data_ready_handler;
	k_spin_unlock(&seesaw_data.lock, key);

	if (!handler) {
		return;
	}

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};

	handler(DEVICE_GET(seesaw), &trig);

	key = k_spin_lock(&seesaw_data.lock);
	if (seesaw_data.data_ready_handler) {
		err = gpio_pin_enable_callback(seesaw_data.irq_gpio_dev,
					       SEESAW_IRQ_GPIO_PIN);
	}
	k_spin_unlock(&seesaw_data.lock, key);

	if (unlikely(err)) {
		LOG_ERR("Cannot re-enable IRQ");
		k_panic();
	}
}

static int seesaw_async_init_verify_id(struct seesaw_data *dev_data)
{
	int err;

	u8_t product_id;
	err = reg_read(dev_data, SEESAW_REG_PRODUCT_ID, &product_id);
	if (err) {
		LOG_ERR("Cannot obtain product ID");
		return err;
	}

	LOG_INF("Product ID: 0x%" PRIx8, product_id);
	if (product_id != SEESAW_PRODUCT_ID) {
		LOG_ERR("Invalid product ID (0x%" PRIx8")!", product_id);
		return -EIO;
	}

	return err;
}

static int seesaw_async_init_configure(struct seesaw_data *dev_data)
{
	int err;

	err = ENOSYS;

	return err;
}

static void seesaw_async_init(struct k_work *work)
{
	struct seesaw_data *dev_data = &seesaw_data;

	ARG_UNUSED(work);

	LOG_DBG("SeeSaw async init step %d", dev_data->async_init_step);

	dev_data->err = async_init_fn[dev_data->async_init_step](dev_data);
	if (dev_data->err) {
		LOG_ERR("SeeSaw initialisation failed");
	} else {
		dev_data->async_init_step++;

		if (dev_data->async_init_step == ASYNC_INIT_STEP_COUNT) {
			dev_data->ready = true;
			LOG_INF("SeeSaw initialised");
		} else {
			k_delayed_work_submit(&dev_data->init_work,
					      async_init_delay[dev_data->async_init_step]);
		}
	}
}

static int seesaw_init_irq(struct seesaw_data *dev_data)
{
	int err;

	dev_data->irq_gpio_dev =
		device_get_binding(SEESAW_IRQ_GPIO_DEV_NAME);
	if (!dev_data->irq_gpio_dev) {
		LOG_ERR("Cannot get IRQ GPIO device");
		return -ENXIO;
	}

	err = gpio_pin_configure(dev_data->irq_gpio_dev,
				 SEESAW_IRQ_GPIO_PIN,
				 GPIO_DIR_IN | GPIO_INT | GPIO_PUD_PULL_UP |
				 GPIO_INT_LEVEL | GPIO_INT_ACTIVE_LOW);
	if (err) {
		LOG_ERR("Cannot configure IRQ GPIO");
		return err;
	}

	gpio_init_callback(&dev_data->irq_gpio_cb, irq_handler,
			   BIT(SEESAW_IRQ_GPIO_PIN));

	err = gpio_add_callback(dev_data->irq_gpio_dev, &dev_data->irq_gpio_cb);
	if (err) {
		LOG_ERR("Cannot add IRQ GPIO callback");
	}

	return err;
}

static int seesaw_init_i2c(struct seesaw_data *dev_data)
{
	dev_data->i2c_dev = device_get_binding(SEESAW_I2C_DEV_NAME);
	if (!dev_data->i2c_dev) {
		LOG_ERR("Cannot get I2C device");
		return -ENXIO;
	}

	return 0;
}

static int seesaw_init(struct device *dev)
{
	struct seesaw_data *dev_data = &seesaw_data;
	int err;

	ARG_UNUSED(dev);

	k_work_init(&dev_data->trigger_handler_work, trigger_handler);

	err = seesaw_init_irq(dev_data);
	if (err) {
		return err;
	}

	err = seesaw_init_i2c(dev_data);
	if (err) {
		return err;
	}

	k_delayed_work_init(&dev_data->init_work, seesaw_async_init);

	k_delayed_work_submit(&dev_data->init_work,
			      seesaw_init_delay[dev_data->async_init_step]);

	return err;
}

static int seesaw_sample_fetch(struct device *dev, enum sensor_channel chan)
{
}

static int seesaw_channel_get(struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
}

static int seesaw_trigger_set(struct device *dev,
			      const struct sensor_trigger *trig,
			      sensor_trigger_handler_t handler)
{
}

static int seesaw_attr_set(struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
}

static const struct sensor_driver_api seesaw_driver_api = {
	.sample_fetch = seesaw_sample_fetch,
	.channel_get  = seesaw_channel_get,
	.trigger_set  = seesaw_trigger_set,
	.attr_set     = seesaw_attr_set,
};

DEVICE_AND_API_INIT(seesaw, DT_INST_0_ADAFRUIT_SEESAW_LABEL, seesaw_init,
		    NULL, NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &seesaw_driver_api);
