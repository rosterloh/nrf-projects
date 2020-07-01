#include <zephyr.h>
#include <drivers/sensor.h>
#include <drivers/gpio.h>

#define MODULE env_sensors
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_NW_SNS_DISPLAY_LOG_LEVEL);

#define PMS5003_ENABLE_PIN	4	/* D10 */
#define PMS5003_RESET_PIN	3	/* D11 */

static struct device *bme;
static struct device *pms;
static struct device *gpio_dev;
static struct k_delayed_work sensors_poll;


static void sensors_poll_fn(struct k_work *work)
{
	k_delayed_work_submit(&sensors_poll,
			      K_MSEC(CONFIG_GEN8_CHARGER_CHECK_POLL_INTERVAL_MS));
}

static int sensors_init(void)
{
	int err = 0;

	bme = device_get_binding(DT_LABEL(DT_INST(0, bosch_bme280)));
	if (!bme) {
		LOG_ERR("Could not get pointer to %s",
			DT_LABEL(DT_INST(0, bosch_bme280)));
		err = -ENXIO;
		return err;
	}

	gpio_dev = device_get_binding(DT_LABEL(DT_NODELABEL(gpioc)));
	if (!gpio_dev) {
		LOG_ERR("Could not get pointer to %s",
			DT_LABEL(DT_NODELABEL(gpioc)));
		err = -ENXIO;
		return err;
	}

	gpio_pin_configure(gpio_dev, PMS5003_ENABLE_PIN, GPIO_OUTPUT_HIGH);
	gpio_pin_configure(gpio_dev, PMS5003_RESET_PIN, GPIO_OUTPUT_HIGH);

	pms = device_get_binding(DT_LABEL(DT_INST(0, plantower_pms7003)));
	if (!pms) {
		LOG_ERR("Could not get pointer to %s",
			DT_LABEL(DT_INST(0, plantower_pms7003)));
		err = -ENXIO;
		return err;
	}

	k_delayed_work_init(&sensors_poll, sensors_poll_fn);
	k_delayed_work_submit(&sensors_poll, K_NO_WAIT);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialised;

			__ASSERT_NO_MSG(!initialised);
			initialised = true;

			if (sensors_init()) {
				LOG_ERR("Cannot initialise");
				module_set_state(MODULE_STATE_ERROR);
			} else {
				module_set_state(MODULE_STATE_READY);
			}
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
