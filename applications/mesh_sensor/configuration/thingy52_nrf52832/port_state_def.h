#include <soc.h>

#include "port_state.h"

/* This configuration file is included only once from board module and holds
 * information about default pin states set while board is on and off.
 */

/* This structure enforces the header file is included only once in the build.
 * Violating this requirement triggers a multiple definition error at link time.
 */
const struct {} port_state_def_include_once;


static const struct pin_state port0_on[] = {
};

static const struct pin_state port1_on[] = {
	{8, 1}, // MPU_PWR_CTRL
	{9, 1}  // CCS_PWR_CTRL
};

static const struct pin_state port0_off[] = {
};

static const struct pin_state port1_off[] = {
	{8, 0}, // MPU_PWR_CTRL
	{9, 0}  // CCS_PWR_CTRL
};

static const struct port_state port_state_on[] = {
	{
		.name     = DT_LABEL(DT_NODELABEL(gpio0)),
		.ps       = port0_on,
		.ps_count = ARRAY_SIZE(port0_on),
	},
	{
		.name     = DT_LABEL(DT_NODELABEL(sx1509b)),
		.ps       = port1_on,
		.ps_count = ARRAY_SIZE(port1_on),
	}
};

static const struct port_state port_state_off[] = {
	{
		.name     = DT_LABEL(DT_NODELABEL(gpio0)),
		.ps       = port0_off,
		.ps_count = ARRAY_SIZE(port0_off),
	},
	{
		.name     = DT_LABEL(DT_NODELABEL(sx1509b)),
		.ps       = port1_off,
		.ps_count = ARRAY_SIZE(port1_off),
	}
};
