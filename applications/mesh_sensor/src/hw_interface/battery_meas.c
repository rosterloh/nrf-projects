#include <zephyr/types.h>

#include <soc.h>
#include <device.h>
#include <drivers/adc.h>
#include <drivers/gpio.h>
#include <sys/atomic.h>

#include <hal/nrf_saadc.h>

#include "event_manager.h"
#include "power_event.h"
#include "battery_event.h"
#include "battery_def.h"

#define MODULE battery_meas
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_MESH_SENSOR_BATTERY_MEAS_LOG_LEVEL);

#define ADC_DEVICE_NAME		DT_ADC_0_NAME
#define ADC_RESOLUTION		12
#define ADC_OVERSAMPLING	4 /* 2^ADC_OVERSAMPLING samples are averaged */
#define ADC_MAX 		4096
#define ADC_GAIN		BATTERY_MEAS_ADC_GAIN
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_REF_INTERNAL_MV	600UL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10)
#define ADC_CHANNEL_ID		0
#define ADC_CHANNEL_INPUT	BATTERY_MEAS_ADC_INPUT
/* ADC asynchronous read is scheduled on odd works. Value read happens during
 * even works. This is done to avoid creating a thread for battery monitor.
 */
#define BATTERY_WORK_INTERVAL	(CONFIG_MESH_SENSOR_BATTERY_MEAS_POLL_INTERVAL_MS / 2)

#if IS_ENABLED(CONFIG_MESH_SENSOR_BATTERY_MEAS_HAS_VOLTAGE_DIVIDER)
#define BATTERY_VOLTAGE(sample)	(sample * BATTERY_MEAS_VOLTAGE_GAIN	\
		* ADC_REF_INTERNAL_MV					\
		* (CONFIG_MESH_SENSOR_BATTERY_MEAS_VOLTAGE_DIVIDER_UPPER	\
		+ CONFIG_MESH_SENSOR_BATTERY_MEAS_VOLTAGE_DIVIDER_LOWER)	\
		/ CONFIG_MESH_SENSOR_BATTERY_MEAS_VOLTAGE_DIVIDER_LOWER / ADC_MAX)
#else
#define BATTERY_VOLTAGE(sample) (sample * BATTERY_MEAS_VOLTAGE_GAIN	\
				 * ADC_REF_INTERNAL_MV / ADC_MAX)
#endif

#define VBATT DT_PATH(vbatt)

static s16_t adc_buffer;
static bool adc_async_read_pending;

static struct k_delayed_work battery_lvl_read;
static struct k_poll_signal async_sig = K_POLL_SIGNAL_INITIALIZER(async_sig);
static struct k_poll_event  async_evt =
	K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
				 K_POLL_MODE_NOTIFY_ONLY,
				 &async_sig);

struct io_channel_config {
	const char *label;
	u8_t channel;
};

struct gpio_channel_config {
	const char *label;
	u8_t pin;
	u8_t flags;
};

struct divider_config {
	const struct io_channel_config io_channel;
	const struct gpio_channel_config power_gpios;
	const u32_t output_ohm;
	const u32_t full_ohm;
};

static const struct divider_config divider_config = {
	.io_channel = {
		DT_IO_CHANNELS_LABEL(VBATT),
		DT_IO_CHANNELS_INPUT(VBATT),
	},
#if DT_NODE_HAS_PROP(VBATT, power_gpios)
	.power_gpios = {
		DT_GPIO_LABEL(VBATT, power_gpios),
		DT_GPIO_PIN(VBATT, power_gpios),
		DT_GPIO_FLAGS(VBATT, power_gpios),
	},
#endif
	.output_ohm = DT_PROP(VBATT, output_ohms),
	.full_ohm = DT_PROP(VBATT, full_ohms),
};

struct divider_data {
	struct device *adc;
	struct device *gpio;
	struct adc_channel_cfg adc_cfg;
	struct adc_sequence adc_seq;
	s16_t raw;
};
static struct divider_data divider_data;
static int batt_mV;

static atomic_t active;
static bool sampling;

static int init_adc(void)
{
	const struct divider_config *cfg = &divider_config;
	const struct io_channel_config *iocp = &cfg->io_channel;
	const struct gpio_channel_config *gcp = &cfg->power_gpios;
	struct divider_data *ddp = &divider_data;
	struct adc_sequence *asp = &ddp->adc_seq;
	struct adc_channel_cfg *accp = &ddp->adc_cfg;
	int rc;

	ddp->adc = device_get_binding(iocp->label);
	if (ddp->adc == NULL) {
		LOG_ERR("Failed to get ADC %s", iocp->label);
		return -ENOENT;
	}

	if (gcp->label) {
		ddp->gpio = device_get_binding(gcp->label);
		if (ddp->gpio == NULL) {
			LOG_ERR("Failed to get GPIO %s", gcp->label);
			return -ENOENT;
		}
		rc = gpio_pin_configure(ddp->gpio, gcp->pin,
					GPIO_OUTPUT_INACTIVE | gcp->flags);
		if (rc != 0) {
			LOG_ERR("Failed to control feed %s.%u: %d",
				gcp->label, gcp->pin, rc);
			return rc;
		}
	}

	*asp = (struct adc_sequence){
		.channels = BIT(0),
		.buffer = &ddp->raw,
		.buffer_size = sizeof(ddp->raw),
		.oversampling = 4,
		.calibrate = true,
	};

#ifdef CONFIG_ADC_NRFX_SAADC
	*accp = (struct adc_channel_cfg){
		.gain = BATTERY_ADC_GAIN,
		.reference = ADC_REF_INTERNAL,
		.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40),
		.input_positive = SAADC_CH_PSELP_PSELP_AnalogInput0 + iocp->channel,
	};

	asp->resolution = 14;
#else /* CONFIG_ADC_var */
#error Unsupported ADC
#endif /* CONFIG_ADC_var */

	rc = adc_channel_setup(ddp->adc, accp);
	LOG_INF("Setup AIN%u got %d", iocp->channel, rc);
/*
	// Check if number of elements in LUT is proper
	BUILD_ASSERT(CONFIG_MESH_SENSOR_BATTERY_MEAS_MAX_LEVEL
		     - CONFIG_MESH_SENSOR_BATTERY_MEAS_MIN_LEVEL
		     == (ARRAY_SIZE(battery_voltage_to_soc) - 1)
		     * CONFIG_MESH_SENSOR_VOLTAGE_TO_SOC_DELTA,
		     "Improper number of elements in lookup table");
*/
	return rc;
}

static int battery_monitor_start(void)
{
	const struct divider_data *ddp = &divider_data;
	const struct gpio_channel_config *gcp = &divider_config.power_gpios;

	if (ddp->gpio) {
		gpio_pin_set(ddp->gpio, gcp->pin, 1);
	}

	sampling = true;
	k_delayed_work_submit(&battery_lvl_read,
			      K_MSEC(BATTERY_WORK_INTERVAL));

	return 0;
}

static void battery_monitor_stop(void)
{
	k_delayed_work_cancel(&battery_lvl_read);
	sampling = false;
	int err = 0;
	const struct divider_data *ddp = &divider_data;
	const struct gpio_channel_config *gcp = &divider_config.power_gpios;

	if (ddp->gpio) {
		err = gpio_pin_set(ddp->gpio, gcp->pin, 0);

		if (err) {
			LOG_ERR("Cannot disable battery monitor circuit");
			module_set_state(MODULE_STATE_ERROR);

			return;
		}
	}

	module_set_state(MODULE_STATE_STANDBY);
}

static void battery_lvl_process(void)
{
	const struct battery_level_point *pb = battery_curve;
	u8_t level;

	if (batt_mV >= pb->lvl_mV) {
		/* Measured voltage above highest point, cap at maximum. */
		return pb->lvl_pptt;
	}
	/* Go down to the last point at or below the measured voltage. */
	while ((pb->lvl_pptt > 0)
	       && (batt_mV < pb->lvl_mV)) {
		++pb;
	}
	if (batt_mV < pb->lvl_mV) {
		/* Below lowest point, cap at minimum */
		return pb->lvl_pptt;
	}

	/* Linear interpolation between below and above points. */
	const struct battery_level_point *pa = pb - 1;

	level = pb->lvl_pptt
		+ ((pa->lvl_pptt - pb->lvl_pptt)
		* (batt_mV - pb->lvl_mV)
		/ (pa->lvl_mV - pb->lvl_mV));

	LOG_DBG("[%s]: %d mV; %u pptt", batt_mV, level);

	struct battery_level_event *event = new_battery_level_event();
	event->level = level;

	EVENT_SUBMIT(event);

	LOG_INF("Battery level: %u%% (%u mV)", level, batt_mV);
}

static void battery_lvl_read_fn(struct k_work *work)
{
	struct divider_data *ddp = &divider_data;
	const struct divider_config *dcp = &divider_config;
	struct adc_sequence *sp = &ddp->adc_seq;
	int err;

	err = adc_read(ddp->adc, sp);
	sp->calibrate = false;
	if (err == 0) {
		s32_t val = ddp->raw;

		adc_raw_to_millivolts(adc_ref_internal(ddp->adc),
				      ddp->adc_cfg.gain,
				      sp->resolution,
				      &val);
		batt_mV = val * (u64_t)dcp->full_ohm / dcp->output_ohm;
		LOG_DBG("raw %u ~ %u mV => %d mV", ddp->raw, val, err);

		adc_async_read_pending = false;
		battery_lvl_process();
	}
/*
	if (!adc_async_read_pending) {
		static const struct adc_sequence sequence = {
			.options	= NULL,
			.channels	= BIT(ADC_CHANNEL_ID),
			.buffer		= &adc_buffer,
			.buffer_size	= sizeof(adc_buffer),
			.resolution	= ADC_RESOLUTION,
			.oversampling	= ADC_OVERSAMPLING,
			.calibrate	= false,
		};
		static const struct adc_sequence sequence_calibrate = {
			.options	= NULL,
			.channels	= BIT(ADC_CHANNEL_ID),
			.buffer		= &adc_buffer,
			.buffer_size	= sizeof(adc_buffer),
			.resolution	= ADC_RESOLUTION,
			.oversampling	= ADC_OVERSAMPLING,
			.calibrate	= true,
		};

		static bool calibrated;

		if (likely(calibrated)) {
			err = adc_read_async(adc_dev, &sequence, &async_sig);
		} else {
			err = adc_read_async(adc_dev, &sequence_calibrate,
					     &async_sig);
			calibrated = true;
		}

		if (err) {
			LOG_WRN("Battery level async read failed");
		} else {
			adc_async_read_pending = true;
		}
	} else {
		err = k_poll(&async_evt, 1, K_NO_WAIT);
		if (err) {
			LOG_WRN("Battery level poll failed");
		} else {
			adc_async_read_pending = false;
			battery_lvl_process();
		}
	}
*/
	if (atomic_get(&active) || adc_async_read_pending) {
		k_delayed_work_submit(&battery_lvl_read,
				      K_MSEC(BATTERY_WORK_INTERVAL));
	} else {
		battery_monitor_stop();
	}
}

static int init_fn(void)
{
	int err =  = init_adc();

	if (err) {
		goto error;
	}

	k_delayed_work_init(&battery_lvl_read, battery_lvl_read_fn);

	err = battery_monitor_start();
error:
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

			int err = init_fn();

			if (err) {
				module_set_state(MODULE_STATE_ERROR);
			} else {
				module_set_state(MODULE_STATE_READY);
				atomic_set(&active, true);
			}

			return false;
		}

		return false;
	}

	if (is_wake_up_event(eh)) {
		if (!atomic_get(&active)) {
			atomic_set(&active, true);

			int err = battery_monitor_start();

			if (!err) {
				module_set_state(MODULE_STATE_READY);
			} else {
				module_set_state(MODULE_STATE_ERROR);
			}
		}

		return false;
	}

	if (is_power_down_event(eh)) {
		if (atomic_get(&active)) {
			atomic_set(&active, false);

			if (adc_async_read_pending) {
				__ASSERT_NO_MSG(sampling);
				/* Poll ADC and postpone shutdown */
				k_delayed_work_submit(&battery_lvl_read,
						      K_MSEC(0));
			} else {
				/* No ADC sample left to read, go to standby */
				battery_monitor_stop();
			}
		}

		return sampling;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE_EARLY(MODULE, power_down_event);
EVENT_SUBSCRIBE(MODULE, wake_up_event);
