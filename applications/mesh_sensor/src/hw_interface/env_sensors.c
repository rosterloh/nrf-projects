#include <zephyr/types.h>
#include <device.h>
#include <drivers/sensor.h>

#include "sensor_event.h"

#define MODULE env_sensors
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_MESH_SENSOR_ENV_SENSORS_LOG_LEVEL);

static struct device *hts221_dev;
static struct k_delayed_work read_sensors_work;


static void sensors_read_fn(struct k_work *work)
{
	struct sensor_value val;

	if (sensor_sample_fetch(hts221_dev) <0) {
		LOG_ERR("Sensor sample update error");
		return;
	}

	if (sensor_channel_get(hts221_dev, SENSOR_CHAN_AMBIENT_TEMP, &val) < 0) {
		LOG_ERR("Cannot read HTS221 temperature channel");
		return;
	}

	struct sensor_value_event *event = new_sensor_value_event();
	event->type = SENSOR_CHAN_AMBIENT_TEMP;
	memcpy(event->value, &val, sizeof(val));

	EVENT_SUBMIT(event);

	k_delayed_work_submit(&read_sensors_work, K_SECONDS(10));
}

static int init_sensors(void)
{
	dev = device_get_binding(DT_LABEL(DT_INST(0, st_hts221)));

	if (dev == NULL) {
		LOG_ERR("Could not get HTS221 device");
		return -ENODEV;
	}

	k_delayed_work_init(&read_sensors_work, sensors_read_fn);
	k_delayed_work_submit(&read_sensors_work, K_NO_WAIT);

	return 0;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialised;

			__ASSERT_NO_MSG(!initialised);
			initialised = true;

			if (init_sensors()) {
				LOG_ERR("Cannot initialise");
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
