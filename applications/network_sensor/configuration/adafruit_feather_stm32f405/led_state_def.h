#include "led_state.h"
#include "led_effect.h"

/* This configuration file is included only once from led_state module and holds
 * information about LED effect associated with each state.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} led_state_def_include_once;


/* Map function to LED ID */
static const u8_t led_map[LED_ID_COUNT] = {
	[LED_ID_SYSTEM_STATE] = 0,
};

/* System state LED is unavailable - leave undefined */
static const struct led_effect led_system_state_effect[LED_SYSTEM_STATE_COUNT] = {
	[LED_SYSTEM_STATE_IDLE]     = LED_EFFECT_LED_ON(LED_COLOR(120)),
	[LED_SYSTEM_STATE_CHARGING] = LED_EFFECT_LED_ON(LED_COLOR(255)),
	[LED_SYSTEM_STATE_ERROR]    = LED_EFFECT_LED_BLINK(200, LED_COLOR(250)),
};
