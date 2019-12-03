#include "hid_keymap.h"
#include "key_id.h"

/* This configuration file is included only once from hid_state module and holds
 * information about mapping between buttons and generated reports.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} hid_keymap_def_include_once;

/*
 * HID keymap. The Consumer Control keys are defined in section 15 of
 * the HID Usage Tables document under the following URL:
 * http://www.usb.org/developers/hidpage/Hut1_12v2.pdf
 */
static const struct hid_keymap hid_keymap[] = {
	{ KEY_ID(0x00, 0x06), 0x0007, IN_REPORT_KEYBOARD_KEYS }, /* D */
        { KEY_ID(0x00, 0x07), 0x0016, IN_REPORT_KEYBOARD_KEYS }, /* S */
        { KEY_ID(0x00, 0x09), 0x0004, IN_REPORT_KEYBOARD_KEYS }, /* A */
        { KEY_ID(0x00, 0x0A), 0x001A, IN_REPORT_KEYBOARD_KEYS }, /* W */
        { KEY_ID(0x00, 0x0E), 0x0014, IN_REPORT_KEYBOARD_KEYS }, /* Q */
};