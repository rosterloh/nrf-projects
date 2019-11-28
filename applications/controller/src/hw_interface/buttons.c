#include <zephyr/types.h>

#include <kernel.h>
#include <soc.h>
#include <device.h>
#include <drivers/gpio.h>
#include <drivers/seesaw.h>
#include <misc/util.h>

#include "key_id.h"
#include "gpio_pins.h"
#include "buttons_def.h"

#include "event_manager.h"
#include "button_event.h"
#include "power_event.h"

#define MODULE buttons
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CONTROLLER_BUTTONS_LOG_LEVEL);

#define SCAN_INTERVAL CONFIG_CONTROLLER_BUTTONS_SCAN_INTERVAL

#define BUTTON_RIGHT 6
#define BUTTON_DOWN  7
#define BUTTON_LEFT  9
#define BUTTON_UP    10
#define BUTTON_SEL   14

u32_t button_mask = (1 << BUTTON_RIGHT) | (1 << BUTTON_DOWN) |
                    (1 << BUTTON_LEFT) | (1 << BUTTON_UP) | (1 << BUTTON_SEL);

enum state {
	STATE_IDLE,
	STATE_ACTIVE,
	STATE_SCANNING,
	STATE_SUSPENDING
};

//static struct device *gpio_devs[ARRAY_SIZE(port_map)];
//static struct gpio_callback gpio_cb[ARRAY_SIZE(port_map)];
static struct device *io_dev;
static struct k_delayed_work button_pressed;
static enum state state;

static int callback_ctrl(bool enable)
{
	int err = 0;

	if (enable) {
		//err = gpio_pin_enable_callback(gpio_devs[row[i].port],
		//			       row[i].pin);
	} else {
		//err = gpio_pin_disable_callback(gpio_devs[row[i].port],
		//				row[i].pin);
	}
	if (!enable) {
		/* Callbacks are disabled but they could fire in the meantime.
		 * Make sure pending work is canceled.
		 */
		k_delayed_work_cancel(&button_pressed);
	}

	return err;
}

static int suspend(void)
{
	int err = -EBUSY;

	switch (state) {
	case STATE_SCANNING:
		state = STATE_SUSPENDING;
		break;

	case STATE_SUSPENDING:
		/* Waiting for scanning to stop */
		break;

	case STATE_ACTIVE:
		state = STATE_IDLE;
		err = callback_ctrl(true);
		break;

	case STATE_IDLE:
		err = -EALREADY;
		break;

	default:
		__ASSERT_NO_MSG(false);
		break;
	}

	return err;
}

static void resume(void)
{
	if (state != STATE_IDLE) {
		/* Already activated. */
		return;
	}

	int err = callback_ctrl(false);
	if (err) {
		LOG_ERR("Cannot disable callbacks");
	} else {
		state = STATE_SCANNING;
	}

	if (err) {
		module_set_state(MODULE_STATE_ERROR);
	} else {
		module_set_state(MODULE_STATE_READY);
	}
}

static void button_pressed_fn(struct k_work *work)
{
	int err = callback_ctrl(false);

	if (err) {
		LOG_ERR("Cannot disable callbacks");
		module_set_state(MODULE_STATE_ERROR);
		return;
	}

	switch (state) {
	case STATE_IDLE:;
		struct wake_up_event *event = new_wake_up_event();
		EVENT_SUBMIT(event);
		break;

	case STATE_ACTIVE:
		state = STATE_SCANNING;
		break;

	case STATE_SCANNING:
	case STATE_SUSPENDING:
	default:
		/* Invalid state */
		__ASSERT_NO_MSG(false);
		break;
	}
}

static void init_fn(void)
{
        io_dev = device_get_binding(DT_INST_0_ADAFRUIT_SEESAW_LABEL);
        if (!io_dev) {
                LOG_ERR("Cannot get io device binding");
                goto error;
        }

        seesaw_gpio_configure(io_dev, button_mask, INPUT_PULLUP);
        seesaw_gpio_interrupts(io_dev, button_mask, 1);

	module_set_state(MODULE_STATE_READY);

	/* Perform initial scan */
	state = STATE_SCANNING;

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

			k_delayed_work_init(&button_pressed, button_pressed_fn);

			init_fn();

			return false;
		}

		return false;
	}

	if (is_wake_up_event(eh)) {
		resume();

		return false;
	}

	if (is_power_down_event(eh)) {
		int err = suspend();

		if (!err) {
			module_set_state(MODULE_STATE_STANDBY);
			return false;
		} else if (err == -EALREADY) {
			/* Already suspended */
			return false;
		} else if (err == -EBUSY) {
			/* Cannot suspend while scanning */
			return true;
		}

		LOG_ERR("Error while suspending");
		module_set_state(MODULE_STATE_ERROR);
		return true;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE_EARLY(MODULE, power_down_event);
EVENT_SUBSCRIBE(MODULE, wake_up_event);