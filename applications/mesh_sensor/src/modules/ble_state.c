#include <zephyr/types.h>
#include <power/reboot.h>
#include <drivers/hwinfo.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh.h>
#include <bluetooth/mesh/models.h>

#include <settings/settings.h>

#include "button_event.h"
#include "sensor_event.h"

#define MODULE ble_state
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_MESH_SENSOR_BLE_STATE_LOG_LEVEL);

static struct k_delayed_work oob_work;
static uint32_t button_press_count;
static uint8_t dev_uuid[16];
static struct sensor_value current_temp;

static int ambient_temp_get(struct bt_mesh_sensor *sensor,
			    struct bt_mesh_msg_ctx *ctx,
			    struct sensor_value *rsp)
{
	// rsp->val1 = current_temp.val1;
	// rsp->val2 = current_temp.val2;

	memcpy(rsp, &current_temp, sizeof(current_temp));

	return 0;
}

static struct bt_mesh_sensor temp_sensor = {
	.type = &bt_mesh_sensor_present_amb_temp,
	.get = ambient_temp_get,
};

static struct bt_mesh_sensor *const sensors[] = {
	&temp_sensor,
};

static struct bt_mesh_sensor_srv sensor_srv =
	BT_MESH_SENSOR_SRV_INIT(sensors, ARRAY_SIZE(sensors));

static int output_number(bt_mesh_output_action_t action, uint32_t number)
{
	if (action == BT_MESH_DISPLAY_NUMBER) {
		LOG_INF("OOB Number: %u", number);
		return 0;
	}
/*
	if (action == BT_MESH_BLINK) {
		LOG_DBG("Blinking %u times", number);
		oob_toggles = number * 2;
		oob_toggle_count = 0;
		k_delayed_work_init(&oob_work, oob_blink_toggle);
		k_delayed_work_submit(&oob_work, 0);
		return 0;
	}
*/
	return -ENOTSUP;
}

static int output_string(const char *string)
{
	LOG_INF("OOB String: %s", string);
	return 0;
}

static void oob_button_timeout(struct k_work *work)
{
	bt_mesh_input_number(button_press_count);
}

static int input(bt_mesh_input_action_t act, uint8_t size)
{
	LOG_DBG("Press a button to set the right number.");
	k_delayed_work_init(&oob_work, oob_button_timeout);
	button_press_count = 0;

	return 0;
}

static void input_complete(void)
{
	k_delayed_work_cancel(&oob_work);
}

static void oob_stop(void)
{
	k_delayed_work_cancel(&oob_work);
}

static void prov_complete(uint16_t net_idx, uint16_t src)
{
	oob_stop();
	LOG_DBG("Prov complete! Addr: 0x%04x", src);
}

static void prov_reset(void)
{
	oob_stop();
	bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
}

static const struct bt_mesh_prov prov = {
	.uuid = dev_uuid,
	.output_size = 1,
	.output_actions = (0
		| BT_MESH_DISPLAY_NUMBER
		| BT_MESH_DISPLAY_STRING
		// | BT_MESH_BLINK
		),
	.output_number = output_number,
	.output_string = output_string,
	.input_size = 1,
	.input = input,
//	.input_actions = BT_MESH_PUSH,
	.complete = prov_complete,
	.input_complete = input_complete,
	.reset = prov_reset,
};

/** Configuration server definition */
static struct bt_mesh_cfg_srv cfg_srv = {
	.relay = IS_ENABLED(CONFIG_BT_MESH_RELAY),
	.beacon = BT_MESH_BEACON_ENABLED,
	.frnd = IS_ENABLED(CONFIG_BT_MESH_FRIEND),
	.gatt_proxy = IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY),
	.default_ttl = 7,

	/* 3 transmissions with 20ms interval */
	.net_transmit = BT_MESH_TRANSMIT(2, 20),
	.relay_retransmit = BT_MESH_TRANSMIT(2, 20),
};

static void attention_on(struct bt_mesh_model *mod)
{
	LOG_DBG("Mesh attention on");
}

static void attention_off(struct bt_mesh_model *mod)
{
	LOG_DBG("Mesh attention off");
}

static const struct bt_mesh_health_srv_cb health_srv_cb = {
	.attn_on = attention_on,
	.attn_off = attention_off,
};

static struct bt_mesh_health_srv health_srv = {
	.cb = &health_srv_cb,
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(
		1, BT_MESH_MODEL_LIST(
			BT_MESH_MODEL_CFG_SRV(&cfg_srv),
			BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
			BT_MESH_MODEL_SENSOR_SRV(&sensor_srv)),
		BT_MESH_MODEL_NONE),
};

static const struct bt_mesh_comp comp = {
	.cid = CONFIG_BT_COMPANY_ID,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

static void bt_ready(int err)
{
	if (err) {
		LOG_ERR("Bluetooth initialisation failed (err %d)", err);
		sys_reboot(SYS_REBOOT_WARM);
	}

	LOG_INF("Bluetooth initialised");

	hwinfo_get_device_id(dev_uuid, sizeof(dev_uuid));

	err = bt_mesh_init(&prov, &comp);
	if (err) {
		LOG_ERR("Initialising mesh failed (err %d)", err);
		return;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	// TODO: Move this to after settings load
	bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);

	module_set_state(MODULE_STATE_READY);
}

static int ble_state_init(void)
{
       return bt_enable(bt_ready);
}

static bool button_event_handler(const struct button_event *event)
{
	if (event->pressed)
		button_press_count++;

	return false;
}

static bool sensor_value_event_handler(const struct sensor_value_event *event)
{
	if (event->type == SENSOR_CHAN_AMBIENT_TEMP)
		current_temp = event->value;

	return false;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_button_event(eh)) {
		return button_event_handler(cast_button_event(eh));
	}

	if (is_sensor_value_event(eh)) {
		return sensor_value_event_handler(cast_sensor_value_event(eh));
	}

	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialised;

			__ASSERT_NO_MSG(!initialised);
			initialised = true;

			if (ble_state_init()) {
				LOG_ERR("Cannot initialise");
			}
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE_EARLY(MODULE, button_event);
EVENT_SUBSCRIBE(MODULE, sensor_value_event);
EVENT_SUBSCRIBE(MODULE, module_state_event);
