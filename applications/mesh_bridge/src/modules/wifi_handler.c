#include <device.h>
#include <drivers/gpio.h>

#define MODULE wifi_handler
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_BRIDGE_UART_LOG_LEVEL);

static struct device *wifi_dev;

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			struct device *boot_mode;
			boot_mode = device_get_binding(DT_LABEL(DT_NODELABEL(gpio0)));
			if (boot_mode < 0) {
				LOG_ERR("%s not available",
					DT_LABEL(DT_NODELABEL(gpio0)));
			}

			gpio_pin_configure(boot_mode, 16, GPIO_OUTPUT_ACTIVE);

			wifi_dev = device_get_binding(DT_LABEL(DT_INST(0, espressif_esp)));
			if (wifi_dev < 0) {
				LOG_ERR("%s not available",
					DT_LABEL(DT_INST(0, espressif_esp)));
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
