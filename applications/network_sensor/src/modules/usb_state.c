#include <zephyr/types.h>
#include <sys/byteorder.h>

#include <usb/usb_device.h>

#define MODULE usb_state
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_NW_SNS_USB_STATE_LOG_LEVEL);

#include "usb_event.h"

static enum usb_state state;
static struct device *usb_dev;


static void broadcast_usb_state(void)
{
	struct usb_state_event *event = new_usb_state_event();

	event->state = state;
	event->id = &state;

	EVENT_SUBMIT(event);
}

static void device_status(enum usb_dc_status_code cb_status, const u8_t *param)
{
	static enum usb_state before_suspend;
	enum usb_state new_state = state;

	switch (cb_status) {
	case USB_DC_CONNECTED:
		if (state != USB_STATE_DISCONNECTED) {
			LOG_WRN("USB_DC_CONNECTED when USB is not disconnected");
		}
		new_state = USB_STATE_POWERED;
		break;

	case USB_DC_DISCONNECTED:
		__ASSERT_NO_MSG(state != USB_STATE_DISCONNECTED);
		new_state = USB_STATE_DISCONNECTED;
		break;

	case USB_DC_CONFIGURED:
		__ASSERT_NO_MSG(state != USB_STATE_DISCONNECTED);
		new_state = USB_STATE_ACTIVE;
		break;

	case USB_DC_RESET:
		__ASSERT_NO_MSG(state != USB_STATE_DISCONNECTED);
		if (state == USB_STATE_SUSPENDED) {
			LOG_WRN("USB reset in suspended state, ignoring");
		} else {
			new_state = USB_STATE_POWERED;
		}
		break;

	case USB_DC_SUSPEND:
		if (state == USB_STATE_DISCONNECTED) {
			/* Due to the way USB driver and stack are written
			 * some events may be issued before application
			 * connect its callback.
			 * We assume that device was powered.
			 */
			state = USB_STATE_POWERED;
			LOG_WRN("USB suspended while disconnected");
		}
		before_suspend = state;
		new_state = USB_STATE_SUSPENDED;
		LOG_WRN("USB suspend");
		break;

	case USB_DC_RESUME:
		__ASSERT_NO_MSG(state == USB_STATE_SUSPENDED);
		new_state = before_suspend;
		LOG_WRN("USB resume");
		break;

	case USB_DC_SET_HALT:
	case USB_DC_CLEAR_HALT:
		/* Ignore */
		break;

	case USB_DC_ERROR:
		module_set_state(MODULE_STATE_ERROR);
		break;

	default:
		/* Should not happen */
		__ASSERT_NO_MSG(false);
		break;
	}

	if (new_state != state) {
		// enum usb_state old_state = state;

		state = new_state;

		// if (old_state == USB_STATE_ACTIVE) {
		// 	broadcast_subscription_change();
		// 	reset_pending_report();
		// }

		broadcast_usb_state();

		// if (new_state == USB_STATE_ACTIVE) {
		// 	broadcast_subscription_change();
		// }
	}
}

static int usb_init(void)
{
	usb_dev = device_get_binding("CDC_ACM_0");
	if (usb_dev == NULL) {
		return -ENXIO;
	}

	int err = usb_enable(device_status);
	if (err) {
		LOG_ERR("Cannot enable USB");
		return err;
	}

	return err;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialised;

			__ASSERT_NO_MSG(!initialised);
			initialised = true;

			if (usb_init()) {
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
