/* This configuration file is included only once from leds module and holds
 * information about LED mapping to GPIO devices or PWM channels.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} leds_def_include_once;

/* Mapping the GPIO devices or PWM channels to pin numbers */
static const size_t led_pins[CONFIG_MESH_SENSOR_LED_COUNT]
			    [CONFIG_MESH_SENSOR_LED_COLOUR_COUNT] = {
	{
		DT_GPIO_PIN(DT_ALIAS(led0), gpios),
		DT_GPIO_PIN(DT_ALIAS(led1), gpios),
		DT_GPIO_PIN(DT_ALIAS(led2), gpios),
	}
};
