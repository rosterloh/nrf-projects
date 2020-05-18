#include <zephyr.h>
#include <assert.h>
#include <drivers/pwm.h>
#include <drivers/gpio>

#include "power_event.h"
#include "led_event.h"

#include "leds_def.h"

#define MODULE leds
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_MESH_SENSOR_LED_LOG_LEVEL);

#define LED_ID(led) ((led) - &leds[0])

struct led {
	struct device *dev;

	struct led_colour colour;
	const struct led_effect *effect;
	u16_t effect_step;
	u16_t effect_substep;

	struct k_delayed_work work;
};

static struct led leds[CONFIG_MESH_SENSOR_LED_COUNT];


static void led_out(struct led *led, struct led_colour *colour)
{
	for (size_t i = 0; i < ARRAY_SIZE(colour->c); i++) {
#if CONFIG_BOARD_THINGY52_NRF52832
		if (colour->c[i] > 0) {
			int err = gpio_
		} else {

		}
#else
		int err = pwm_pin_set_usec(led->dev,
					   led_pins[LED_ID(led)][i],
					   CONFIG_MESH_SENSOR_LED_BRIGHTNESS_MAX,
					   colour->c[i],
					   0);
#endif
		if (err) {
			LOG_ERR("Cannot set LED output (err: %d)", err);
		}
	}

}

static void led_off(struct led *led)
{
	struct led_colour nocolour = {0};

	led_out(led, &nocolour);
}

static void work_handler(struct k_work *work)
{
	struct led *led = CONTAINER_OF(work, struct led, work);

	const struct led_effect_step *effect_step =
		&led->effect->steps[led->effect_step];

	__ASSERT_NO_MSG(effect_step->substep_count > 0);
	int substeps_left = effect_step->substep_count - led->effect_substep;

	for (size_t i = 0; i < ARRAY_SIZE(led->colour.c); i++) {
		int diff = (effect_step->colour.c[i] - led->colour.c[i]) /
			substeps_left;
		led->colour.c[i] += diff;
	}
	led_out(led, &led->colour);

	led->effect_substep++;
	if (led->effect_substep == effect_step->substep_count) {
		led->effect_substep = 0;
		led->effect_step++;

		if (led->effect_step == led->effect->step_count) {
			if (led->effect->loop_forever) {
				led->effect_step = 0;
			} else {
				struct led_ready_event *ready_event = new_led_ready_event();

				ready_event->led_id = LED_ID(led);
				ready_event->led_effect = led->effect;

				EVENT_SUBMIT(ready_event);
			}
		}
	}

	if (led->effect_step < led->effect->step_count) {
		s32_t next_delay =
			led->effect->steps[led->effect_step].substep_time;

		k_delayed_work_submit(&led->work, next_delay);
	}
}

static void led_update(struct led *led)
{
	k_delayed_work_cancel(&led->work);

	led->effect_step = 0;
	led->effect_substep = 0;

	if (!led->effect) {
		LOG_WRN("No effect set");
		return;
	}

	__ASSERT_NO_MSG(led->effect->steps);

	if (led->effect->step_count > 0) {
		s32_t next_delay =
			led->effect->steps[led->effect_step].substep_time;

		k_delayed_work_submit(&led->work, next_delay);
	} else {
		LOG_WRN("LED effect with no effect");
	}
}

static int leds_init(void)
{
	const char *dev_name[] = {
#if CONFIG_BOARD_THINGY52_NRF52832
		DT_GPIO_LEDS_LED_0_GPIOS_CONTROLLER,
#endif
#if CONFIG_PWM_0
		DT_NORDIC_NRF_PWM_PWM_0_LABEL,
#endif
#if CONFIG_PWM_1
		DT_NORDIC_NRF_PWM_PWM_1_LABEL,
#endif
#if CONFIG_PWM_2
		DT_NORDIC_NRF_PWM_PWM_2_LABEL,
#endif
#if CONFIG_PWM_3
		DT_NORDIC_NRF_PWM_PWM_3_LABEL,
#endif
	};

	int err = 0;

	BUILD_ASSERT(ARRAY_SIZE(leds) <= ARRAY_SIZE(dev_name),
		     "not enough LED devices");

	for (size_t i = 0; (i < ARRAY_SIZE(leds)) && !err; i++) {
		leds[i].dev = device_get_binding(dev_name[i]);

		if (!leds[i].dev) {
			LOG_ERR("Cannot bind %s", dev_name[i]);
			err = -ENXIO;
		} else {
			k_delayed_work_init(&leds[i].work, work_handler);
			led_update(&leds[i]);
		}
	}

	return err;
}

static void leds_start(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {/*
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
		int err = device_set_power_state(leds[i].dev,
						 DEVICE_PM_ACTIVE_STATE,
						 NULL, NULL);
		if (err) {
			LOG_ERR("PWM enable failed");
		}
#endif*/
		led_update(&leds[i]);
	}
}

static void leds_stop(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(leds); i++) {
		k_delayed_work_cancel(&leds[i].work);

		led_off(&leds[i]);
/*
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
		int err = device_set_power_state(leds[i].dev,
						 DEVICE_PM_SUSPEND_STATE,
						 NULL, NULL);
		if (err) {
			LOG_ERR("PWM disable failed");
		}
#endif*/
	}
}

static bool event_handler(const struct event_header *eh)
{
	static bool initialised;

	if (is_led_event(eh)) {
		const struct led_event *event = cast_led_event(eh);

		__ASSERT_NO_MSG(event->led_id < CONFIG_MESH_SENSOR_LED_COUNT);

		struct led *led = &leds[event->led_id];

		led->effect = event->led_effect;

		if (initialised) {
			led_update(led);
		}

		return false;
	}

	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			int err = leds_init();
			if (!err) {
				initialised = true;
				module_set_state(MODULE_STATE_READY);
			} else {
				module_set_state(MODULE_STATE_ERROR);
			}
		}
		return false;
	}

	if (is_wake_up_event(eh)) {
		if (!initialised) {
			leds_start();
			initialised = true;
			module_set_state(MODULE_STATE_READY);
		}

		return false;
	}

	if (is_power_down_event(eh)) {
		const struct power_down_event *event =
			cast_power_down_event(eh);

		/* Leds should keep working on system error. */
		if (event->error) {
			return false;
		}

		if (initialised) {
			leds_stop();
			initialised = false;
			module_set_state(MODULE_STATE_OFF);
		}

		return initialised;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, led_event);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, power_down_event);
EVENT_SUBSCRIBE(MODULE, wake_up_event);
