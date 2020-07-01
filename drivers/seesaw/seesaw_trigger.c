#include <kernel.h>

#include "seesaw.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(seesaw, CONFIG_SENSOR_LOG_LEVEL);

static void setup_int(struct device *dev,
		      bool enable)
{
	struct seesaw_data *data = DEV_DATA(dev);
	const struct seesaw_config *config = DEV_CFG(dev);
	gpio_flags_t flags = enable
		? GPIO_INT_EDGE_TO_ACTIVE
		: GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure(data->gpio, cfg->int_pin, flags);
}

static void handle_int(struct device *dev)
{
	struct seesaw_data *data = DEV_DATA(dev);

	setup_int(dev, false);

#if defined(CONFIG_SEESAW_TRIGGER_OWN_THREAD)
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_SEESAW_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}


static void process_int(struct device *dev)
{
	struct seesaw_data *data = DEV_DATA(dev);
	const struct seesaw_config *config = DEV_CFG(dev);
	//u8_t status;

	/* Clear the status */
	//if (i2c_reg_read_byte(data->i2c, cfg->i2c_addr,
	//		      ADT7420_REG_STATUS, &status) < 0) {
	//	return;
	//}

	if (data->int_cb != NULL) {
		data->int_cb(dev);
	}

	setup_int(dev, true);

	/* Check for pin that asserted while we were offline */
	int pv = gpio_pin_get(data->gpio, cfg->int_pin);

	if (pv > 0) {
		handle_int(dev);
	}
}

static void seesaw_gpio_callback(struct device *dev,
				 struct gpio_callback *cb, u32_t pins)
{
	struct seesaw_data *data =
		CONTAINER_OF(cb, struct seesaw_data, gpio_cb);

	handle_int(data->dev);
}

#ifdef CONFIG_SEESAW_TRIGGER_OWN_THREAD
static void seesaw_thread(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct seesaw_data *data = DEV_DATA(dev);

	ARG_UNUSED(unused);

	while (42) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		process_int(dev);
	}
}
#endif

#ifdef CONFIG_SEESAW_TRIGGER_GLOBAL_THREAD
static void seesaw_work_cb(struct k_work *work)
{
	struct seesaw_data *drv_data =
		CONTAINER_OF(work, struct seesaw_data, work);

	process_int(drv_data->dev);
}
#endif

int seesaw_set_int_callback(struct device *dev,
			    seesaw_int_callback_t int_cb)
{
	struct seesaw_data *data = DEV_DATA(dev);
        const struct seesaw_config *config = DEV_CFG(dev);

	setup_int(dev, false);

	data->int_cb = int_cb;

        setup_int(dev, true);

	/* Check whether already asserted */
	int pv = gpio_pin_get(data->gpio, cfg->int_pin);

	if (pv > 0) {
		handle_int(dev);
	}

        return 0;
}

int seesaw_init_interrupt(struct device *dev)
{
	struct seesaw_data *data = DEV_DATA(dev);
	const struct seesaw_config *config = DEV_CFG(dev);

	/* setup gpio interrupt */
	data->gpio = device_get_binding(config->int_name);
	if (data->gpio == NULL) {
		LOG_DBG("Failed to get pointer to %s device!",
			config->int_name);
		return -EINVAL;
	}

	gpio_init_callback(&data->gpio_cb,
			   seesaw_gpio_callback,
			   BIT(config->int_pin));

	gpio_pin_configure(data->gpio, config->int_pin,
			   GPIO_INPUT | cfg->int_flags);

	if (gpio_add_callback(data->gpio, &data->gpio_cb) < 0) {
		LOG_DBG("Failed to set gpio callback!");
		return -EIO;
	}

	data->dev = dev;

#if defined(CONFIG_SEESAW_TRIGGER_OWN_THREAD)
	k_sem_init(&data->gpio_sem, 0, UINT_MAX);

	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_SEESAW_THREAD_STACK_SIZE,
			(k_thread_entry_t)seesaw_thread, dev,
			0, NULL, K_PRIO_COOP(CONFIG_SEESAW_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_SEESAW_TRIGGER_GLOBAL_THREAD)
	data->work.handler = seesaw_work_cb;
#endif

	return 0;
}
