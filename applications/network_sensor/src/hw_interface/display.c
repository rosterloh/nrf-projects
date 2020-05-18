#include <zephyr/types.h>
#include <drivers/display.h>
#include <display/cfb.h>

#include "button_event.h"

#define MODULE display
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_NW_SNS_DISPLAY_LOG_LEVEL);

#if defined(CONFIG_SSD1306)
#define DISPLAY_DRIVER		"SSD1306"
#endif

static struct device *display_dev;
static u8_t ppt;

static int display_init(void)
{
	display_dev = device_get_binding(DISPLAY_DRIVER);
	if (display_dev == NULL) {
		LOG_ERR("Display not found");
		return -ENXIO;
	}

	int err = cfb_framebuffer_init(display_dev);
	if (err) {
		LOG_ERR("Cannot enable framebuffer");
		return err;
	}

	err = cfb_framebuffer_invert(display_dev);

	err = cfb_framebuffer_clear(display_dev, true);

	display_blanking_off(display_dev);

	ppt = cfb_get_display_parameter(display_dev, CFB_DISPLAY_PPT);

	LOG_INF("x_res %d, y_res %d, ppt %d, rows %d, cols %d",
		cfb_get_display_parameter(display_dev, CFB_DISPLAY_WIDTH),
	        cfb_get_display_parameter(display_dev, CFB_DISPLAY_HEIGH),
	        ppt,
	        cfb_get_display_parameter(display_dev, CFB_DISPLAY_ROWS),
	        cfb_get_display_parameter(display_dev, CFB_DISPLAY_COLS));


	return err;
}

static bool handle_button_event(const struct button_event *event)
{
	LOG_DBG("Key %u %s", event->key_id, event->pressed ? "pressed" : "released");

	cfb_framebuffer_clear(display_dev, false);

	if (event->key_id == 0) {
	 	cfb_print(display_dev, "A ", 0, ppt);
	} else if (event->key_id == 1) {
		cfb_print(display_dev, "B ", 0, ppt);
	} else if (event->key_id == 2) {
		cfb_print(display_dev, "C ", 0, ppt);
	} else {
		cfb_print(display_dev, "? ", 0, ppt);
	}

	if (event->pressed) {
		cfb_print(display_dev, "pressed", 2 * ppt, ppt);
	} else {
		cfb_print(display_dev, "released", 2 * ppt, ppt);
	}

	cfb_framebuffer_finalize(display_dev);

	return false;
}

static bool event_handler(const struct event_header *eh)
{
	if (!IS_ENABLED(CONFIG_NW_SNS_BUTTONS_NONE) &&
	    is_button_event(eh)) {
		return handle_button_event(cast_button_event(eh));
	}

	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialised;

			__ASSERT_NO_MSG(!initialised);
			initialised = true;

			if (display_init()) {
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
EVENT_SUBSCRIBE(MODULE, button_event);
