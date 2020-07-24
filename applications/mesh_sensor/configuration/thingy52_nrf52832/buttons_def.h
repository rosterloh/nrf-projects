#include "gpio_pins.h"

/* This configuration file is included only once from button module and holds
 * information about pins forming keyboard matric.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} buttons_def_include_once;

static const struct gpio_pin button_pins[] = {
	{ .port = 0, .pin = 11 },
};
