/* This configuration file is included only once from battery level measuring
 * module and holds information about battery characteristic.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} battery_def_include_once;

#define BATTERY_MEAS_ADC_INPUT		NRF_SAADC_INPUT_AIN4
#define BATTERY_MEAS_ADC_GAIN		ADC_GAIN_1
#define BATTERY_MEAS_VOLTAGE_GAIN	4

/* Converting measured battery voltage[mV] to State of Charge[%].
 * First element corresponds to CONFIG_MESH_SENSOR_BATTERY_MIN_LEVEL.
 * Each element is CONFIG_MESH_SENSOR_VOLTAGE_TO_SOC_DELTA higher than previous.
 * Defined separately for every configuration.
 */
static const u8_t battery_voltage_to_soc[] = {
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,
 1,  1,  2,  2,  2,  2,  3,  3,  3,  4,  4,  5,  5,  6,  6,  7,  8,  9,  9, 10,
11, 13, 14, 15, 16, 18, 19, 21, 23, 24, 26, 28, 31, 33, 35, 37, 40, 42, 45, 47,
50, 52, 54, 57, 59, 62, 64, 66, 68, 71, 73, 75, 76, 78, 80, 81, 83, 84, 85, 86,
88, 89, 90, 90, 91, 92, 93, 93, 94, 94, 95, 95, 96, 96, 96, 97, 97, 97, 97, 98,
98, 98, 98, 98, 98, 98, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 100,
100
};

/** A point in a battery discharge curve sequence.
 *
 * A discharge curve is defined as a sequence of these points, where
 * the first point has #lvl_pptt set to 10000 and the last point has
 * #lvl_pptt set to zero.  Both #lvl_pptt and #lvl_mV should be
 * monotonic decreasing within the sequence.
 */
struct battery_level_point {
	/** Remaining life at #lvl_mV. */
	uint16_t lvl_pptt;

	/** Battery voltage at #lvl_pptt remaining life. */
	uint16_t lvl_mV;
};

static const struct battery_level_point battery_curve[] = {
	/* "Curve" here eyeballed from captured data for a full load
	 * that started with a charge of 3.96 V and dropped about
	 * linearly to 3.58 V over 15 hours.  It then dropped rapidly
	 * to 3.10 V over one hour, at which point it stopped
	 * transmitting.
	 *
	 * Based on eyeball comparisons we'll say that 15/16 of life
	 * goes between 3.95 and 3.55 V, and 1/16 goes between 3.55 V
	 * and 3.1 V.
	 */

	{ 10000, 3950 },
	{ 625, 3550 },
	{ 0, 3100 },
};
