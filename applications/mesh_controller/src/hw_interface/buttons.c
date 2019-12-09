
#include <kernel.h>
#include <soc.h>
#include <device.h>
#include <gpio.h>
#include <misc/util.h>

#include "key_id.h"
#include "gpio_pins.h"
#include "buttons_def.h"

#include "event_manager.h"
#include "button_event.h"

#define MODULE buttons
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_MESH_CTL_BUTTONS_LOG_LEVEL);

#define SCAN_INTERVAL CONFIG_MESH_CTL_BUTTONS_SCAN_INTERVAL

/* For directly connected GPIO, scan rows once. */
#define COLUMNS MAX(ARRAY_SIZE(col), 1)
#define MAX_DEBOUNCE (3)

static struct device *gpio_devs[ARRAY_SIZE(port_map)];
static struct k_delayed_work matrix_scan;
static u8_t debounce_count[COLUMNS];

static void scan_fn(struct k_work *work)
{
        static u8_t current = 0;
	u32_t val;

        gpio_pin_write(gpio_devs[col[current].port], col[current].pin, 0);

	if (gpio_pin_read(gpio_devs[row[0].port], row[0].pin, &val)) {
                LOG_ERR("Cannot get pin");
                goto error;
        }

        if (!val) {
                if (debounce_count[current] < MAX_DEBOUNCE)
                {
                        debounce_count[current]++;
                        if (debounce_count[current] == MAX_DEBOUNCE)
                        {
                                struct button_event *event = new_button_event();
                                event->key_id = KEY_ID(0, current);
				event->pressed = true;
				EVENT_SUBMIT(event);
                        }
                }
        } else {
                // otherwise, button is released
                if (debounce_count[current] > 0)
                {
                        debounce_count[current]--;
                        if (debounce_count[current] == 0)
                        {
                                struct button_event *event = new_button_event();
                                event->key_id = KEY_ID(0, current);
				event->pressed = false;
				EVENT_SUBMIT(event);
                        }
                }
        }

        gpio_pin_write(gpio_devs[col[current].port], col[current].pin, 1);

        current++;
        if (current >= COLUMNS)
        {
                current = 0;
        }
 
        /* Schedule next scan */
        k_delayed_work_submit(&matrix_scan, SCAN_INTERVAL);

	return;

error:
	module_set_state(MODULE_STATE_ERROR);
}

static void init_fn(void)
{
        int err = 0;

	/* Setup GPIO configuration */
	for (size_t i = 0; i < ARRAY_SIZE(port_map); i++) {
		if (!port_map[i]) {
			/* Skip non-existing ports */
			continue;
		}
		gpio_devs[i] = device_get_binding(port_map[i]);
		if (!gpio_devs[i]) {
			LOG_ERR("Cannot get GPIO device binding");
			goto error;
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(col); i++) {
		__ASSERT_NO_MSG(col[i].port < ARRAY_SIZE(port_map));
		__ASSERT_NO_MSG(gpio_devs[col[i].port] != NULL);

		err = gpio_pin_configure(gpio_devs[col[i].port],
					     col[i].pin, GPIO_DIR_OUT);

		if (err) {
			LOG_ERR("Cannot configure cols");
			goto error;
		} else {
                       gpio_pin_write(gpio_devs[col[i].port],
				      col[i].pin, 1); 
                }
	}

	int flags = GPIO_DIR_IN | GPIO_PUD_PULL_UP | GPIO_INT_ACTIVE_LOW;
        for (size_t i = 0; (i < ARRAY_SIZE(row)) && !err; i++) {
		__ASSERT_NO_MSG(row[i].port < ARRAY_SIZE(port_map));
		__ASSERT_NO_MSG(port_map[row[i].port] != NULL);

		err = gpio_pin_configure(gpio_devs[row[i].port], row[i].pin,
				         flags);
	}

        if (err) {
                LOG_ERR("Cannot configure rows");
                goto error;
        }

	module_set_state(MODULE_STATE_READY);

	scan_fn(NULL);

	return;

error:
	module_set_state(MODULE_STATE_ERROR);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialised;

			__ASSERT_NO_MSG(!initialised);
			initialised = true;

			k_delayed_work_init(&matrix_scan, scan_fn);

			init_fn();

			return false;
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);