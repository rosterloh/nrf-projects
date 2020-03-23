#include <zephyr/types.h>

#include <kernel.h>
#include <soc.h>
#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>

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

#define BUTTON_A 6
#define BUTTON_B 7
#define BUTTON_Y 9
#define BUTTON_X 10
#define BUTTON_SEL 14

u32_t button_mask = (1 << BUTTON_A) | (1 << BUTTON_B) |
                    (1 << BUTTON_Y) | (1 << BUTTON_X) | (1 << BUTTON_SEL);
u8_t buttons[5] = { BUTTON_A, BUTTON_B, BUTTON_Y, BUTTON_X, BUTTON_SEL }; 
u32_t button_state = 0;
u32_t prev_state = 0;

enum state {
	STATE_IDLE,
	STATE_ACTIVE,
	STATE_SCANNING,
	STATE_SUSPENDING
};

static struct device *io_dev;
static struct k_delayed_work button_pressed;
static enum state state;

static void button_handler(struct device *dev)
{
        seesaw_read_digital(dev, button_mask, &button_state);

        k_delayed_work_submit(&button_pressed, 0); 
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
		//err = callback_ctrl(true);
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

	int err = 0;//callback_ctrl(false);
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
        /* Emit event for any key state change */
	size_t evt_limit = 0;

        for (size_t i = 0; i < 5; i++) {
                bool is_pressed = !(button_state & BIT(buttons[i]));
                bool was_pressed = prev_state & BIT(buttons[i]);

                if ((is_pressed != was_pressed) &&
                    (evt_limit < CONFIG_CONTROLLER_BUTTONS_EVENT_LIMIT)) {
                        struct button_event *event = new_button_event();

                        event->key_id = KEY_ID(0, buttons[i]);

                        event->pressed = is_pressed;
                        EVENT_SUBMIT(event);

                        evt_limit++;

                        WRITE_BIT(prev_state, buttons[i], is_pressed);
                }
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
        seesaw_int_set(io_dev, button_handler);

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
