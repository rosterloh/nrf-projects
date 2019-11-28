#include <device.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include "seesaw.h"

extern struct seesaw_data seesaw_driver;

#include <logging/log.h>
LOG_MODULE_DECLARE(seesaw, CONFIG_SENSOR_LOG_LEVEL);

int seesaw_attr_set(struct device *dev,
		     enum sensor_channel chan,
		     enum sensor_attribute attr,
		     const struct sensor_value *val)
{
/*	struct seesaw_data *data = DEV_DATA(dev);
	const struct seesaw_config *config = DEV_CFG(dev);
	s16_t int_level = (val->val1 * 1000000 + val->val2) /
			  SEESAW_TREG_LSB_SCALING;
	u8_t intl_reg;
	u8_t inth_reg;
*/
	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}
/*
	LOG_DBG("set threshold to %d", int_level);

	if (attr == SENSOR_ATTR_UPPER_THRESH) {
		intl_reg = SEESAW_INTHL;
		inth_reg = SEESAW_INTHH;
	} else if (attr == SENSOR_ATTR_LOWER_THRESH) {
		intl_reg = SEESAW_INTLL;
		inth_reg = SEESAW_INTLH;
	} else {
		return -ENOTSUP;
	}

	if (i2c_reg_write_byte(data->i2c, config->i2c_address,
			       intl_reg, (u8_t)int_level)) {
		LOG_DBG("Failed to set INTxL attribute!");
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, config->i2c_address,
			       inth_reg, (u8_t)(int_level >> 8))) {
		LOG_DBG("Failed to set INTxH attribute!");
		return -EIO;
	}
*/
	return 0;
}

static void seesaw_gpio_callback(struct device *dev,
				  struct gpio_callback *cb, u32_t pins)
{
	struct seesaw_data *drv_data =
		CONTAINER_OF(cb, struct seesaw_data, gpio_cb);
	const struct seesaw_config *config = DEV_CFG(dev);

	gpio_pin_disable_callback(dev, config->gpio_pin);

#if defined(CONFIG_SEESAW_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_SEESAW_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void seesaw_thread_cb(void *arg)
{
	struct device *dev = arg;
	struct seesaw_data *data = DEV_DATA(dev);
	const struct seesaw_config *config = DEV_CFG(dev);
/*	u8_t status;

	if (i2c_reg_read_byte(data->i2c, config->i2c_address,
			      SEESAW_STAT, &status) < 0) {
		return;
	}

	if (data->int_handler != NULL) {
		data->int_handler(dev, &data->int_trigger);
	}
*/
	gpio_pin_enable_callback(data->gpio, config->gpio_pin);
}

#ifdef CONFIG_SEESAW_TRIGGER_OWN_THREAD
static void seesaw_thread(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct seesaw_data *data = DEV_DATA(dev);

	ARG_UNUSED(unused);

	while (42) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		seesaw_thread_cb(dev);
	}
}
#endif

#ifdef CONFIG_SEESAW_TRIGGER_GLOBAL_THREAD
static void seesaw_work_cb(struct k_work *work)
{
	struct seesaw_data *drv_data =
		CONTAINER_OF(work, struct seesaw_data, work);
	seesaw_thread_cb(drv_data->dev);
}
#endif

int seesaw_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct seesaw_data *data = DEV_DATA(dev);
	const struct seesaw_config *config = DEV_CFG(dev);
/*
	if (i2c_reg_write_byte(data->i2c, config->i2c_address,
			       SEESAW_INTC, SEESAW_INTC_DISABLED)) {
		return -EIO;
	}
*/
	gpio_pin_disable_callback(data->gpio, config->gpio_pin);

	if (trig->type == SENSOR_TRIG_THRESHOLD) {
		data->int_handler = handler;
		data->int_trigger = *trig;
	} else {
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	gpio_pin_enable_callback(data->gpio, config->gpio_pin);
/*
	if (i2c_reg_write_byte(data->i2c, config->i2c_address,
			       SEESAW_INTC, SEESAW_INTC_ABS_MODE)) {
		return -EIO;
	}
*/
	return 0;
}

int seesaw_init_interrupt(struct device *dev)
{
	struct seesaw_data *data = DEV_DATA(dev);
	const struct seesaw_config *config = DEV_CFG(dev);

	/* setup gpio interrupt */
	data->gpio = device_get_binding(config->gpio_name);
	if (data->gpio == NULL) {
		LOG_DBG("Failed to get pointer to %s device!",
			config->gpio_name);
		return -EINVAL;
	}

	gpio_pin_configure(data->gpio, config->gpio_pin,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_LOW | GPIO_INT_DEBOUNCE);

	gpio_init_callback(&data->gpio_cb,
			   seesaw_gpio_callback,
			   BIT(config->gpio_pin));

	if (gpio_add_callback(data->gpio, &data->gpio_cb) < 0) {
		LOG_DBG("Failed to set gpio callback!");
		return -EIO;
	}

#if defined(CONFIG_SEESAW_TRIGGER_OWN_THREAD)
	k_sem_init(&data->gpio_sem, 0, UINT_MAX);

	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_SEESAW_THREAD_STACK_SIZE,
			(k_thread_entry_t)seesaw_thread, dev,
			0, NULL, K_PRIO_COOP(CONFIG_SEESAW_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_SEESAW_TRIGGER_GLOBAL_THREAD)
	data->work.handler = seesaw_work_cb;
	data->dev = dev;
#endif

	return 0;
}