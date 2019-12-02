#include <device.h>
#include <sys/util.h>
#include <kernel.h>
#include "seesaw.h"

extern struct seesaw_data seesaw_driver;

#include <logging/log.h>
LOG_MODULE_DECLARE(seesaw, CONFIG_SENSOR_LOG_LEVEL);

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

	if (data->int_cb) {
		data->int_cb(dev);
	}

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

int seesaw_set_int_callback(struct device *dev,
			    seesaw_int_callback_t int_cb)
{
	struct seesaw_data *data = DEV_DATA(dev);
        const struct seesaw_config *config = DEV_CFG(dev);

	data->int_cb = int_cb;

        gpio_pin_enable_callback(data->gpio, config->gpio_pin);

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