/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**@file
 * @defgroup nrf_bt_scan BT Scanning module
 * @{
 * @brief BT Scanning module
 *
 * @details The Scanning Module handles the BLE scanning for
 *          your application. The module offers several criteria
 *          for filtering the devices available for connection,
 *          and it can also work in the simple mode without using the filtering.
 *          If an event handler is provided, your main application can react to
 *          a filter match. The module can also be configured to automatically
 *          connect after it matches a filter.
 *
 * @note The Scanning Module also supports applications with a
 *       multicentral link.
 */


#ifndef BT_SCAN_H_
#define BT_SCAN_H_

#include <zephyr/types.h>
#include <misc/slist.h>
#include <bluetooth/hci.h>
#include <bluetooth/uuid.h>
#include <bluetooth/conn.h>

#ifdef __cplusplus
extern "C" {
#endif


/**@defgroup BT_SCAN_FILTER_MODE Filter modes
 * @{
 */

/**@brief Filters the device name. */
#define BT_SCAN_NAME_FILTER 0x01

/**@brief Filters the device address. */
#define BT_SCAN_ADDR_FILTER 0x02

/**@brief Filters the UUID. */
#define BT_SCAN_UUID_FILTER 0x04

/**@brief Filters the appearance. */
#define BT_SCAN_APPEARANCE_FILTER 0x08

/**@brief Filters the device short name. */
#define BT_SCAN_SHORT_NAME_FILTER 0x10

/**@brief Filters the manufacturer data. */
#define BT_SCAN_MANUFACTURER_DATA_FILTER 0x20

/**@brief Uses the combination of all filters. */
#define BT_SCAN_ALL_FILTER 0x3F
/* @} */


/**@brief Scan types.
 */
enum bt_scan_type {
	/** Active scanning. */
	BT_SCAN_TYPE_SCAN_PASSIVE,

	/** Passive scanning. */
	BT_SCAN_TYPE_SCAN_ACTIVE
};

/**@brief Types of filters.
 */
enum bt_scan_filter_type {
	/** Filter for names. */
	BT_SCAN_FILTER_TYPE_NAME,

	/** Filter for short names. */
	BT_SCAN_FILTER_TYPE_SHORT_NAME,

	/** Filter for addresses. */
	BT_SCAN_FILTER_TYPE_ADDR,

	/** Filter for UUIDs. */
	BT_SCAN_FILTER_TYPE_UUID,

	/** Filter for appearances. */
	BT_SCAN_FILTER_TYPE_APPEARANCE,

	/** Filter for manufacturer data. */
	BT_SCAN_FILTER_TYPE_MANUFACTURER_DATA,
};

/**@brief Filter information structure.
 *
 * @details It contains information about the number of filters of a given type
 *          and whether they are enabled
 */
struct bt_scan_filter_info {
	/** Filter enabled. */
	bool enabled;

	/** Filter count. */
	u8_t cnt;
};

/**@brief Filter status structure.
 */
struct bt_filter_status {
	/** Filter mode. */
	bool all_mode;

	/** Name filters info. */
	struct bt_scan_filter_info name;

	/** Short name filters info. */
	struct bt_scan_filter_info short_name;

	/** Address filters info. */
	struct bt_scan_filter_info addr;

	/** UUID filters info. */
	struct bt_scan_filter_info uuid;

	/** Appearance filter info. */
	struct bt_scan_filter_info appearance;

	/** Appearance filter info. */
	struct bt_scan_filter_info manufacturer_data;
};

/**@brief Advertising info structure.
 */
struct bt_scan_adv_info {
	/** BLE advertising type. According to
	 * Bluetooth Specification 7.8.5
	 */
	u8_t adv_type;

	/** Received Signal Strength Indication in dBm. */
	s8_t rssi;
};

/**@brief A helper structure to set filters for the name.
 */
struct bt_scan_short_name {
	/** Pointer to the short name. */
	const char *name;

	/** Minimum length of the short name. */
	u8_t min_len;
};

/**@brief A helper structure to set filters for the manufacturer data.
 */
struct bt_scan_manufacturer_data {
	/** Pointer to the manufacturer data. */
	u8_t *data;

	/** Manufacturer data length. */
	u8_t data_len;
};

/**@brief Structure for Scanning Module initialization.
 */
struct bt_scan_init_param {
	/** Scan parameters required to initialize the module.
	 * Can be initialized as NULL. If NULL, the parameters required to
	 * initialize the module are loaded from the static configuration.
	 */
	const struct bt_le_scan_param *scan_param;

	/** If set to true, the module automatically
	 * connects after a filter match.
	 */
	bool connect_if_match;

	/** Connection parameters. Can be initialized as NULL.
	 * If NULL, the default static configuration is used.
	 */
	const struct bt_le_conn_param *conn_param;
};

/**@brief Structure for setting the filter status.
 *
 * @details This structure is used for sending
 *          filter status to the main application.
 */
struct bt_scan_filter_match {
	/** Set to 1 if name filter is matched. */
	u8_t name : 1;

	/** Set to 1 if address filter is matched. */
	u8_t address : 1;

	/** Set to 1 if uuid filter is matched. */
	u8_t uuid : 1;

	/** Set to 1 if appearance filter is matched. */
	u8_t appearance: 1;

	/** Set to 1 if short name filter is matched. */
	u8_t short_name : 1;

	/** Set to 1 if manufacturer data filter is matched. */
	u8_t manufacturer_data : 1;
};

/**@brief Structure contains device data needed to establish
 *        connection and advertising information.
 */
struct bt_scan_device_info {
	/** Information about advertising. */
	struct bt_scan_adv_info adv_info;

	/** Pointer to device BLE address. */
	const bt_addr_le_t *addr;

	/** Connection parameters for LE connection. */
	const struct bt_le_conn_param *conn_param;
};

/** @brief Scanning callback structure.
 *
 *  This structure is used for tracking the state of a scanning.
 *  It is registered with the help of the @ref bt_scan_cb_register() API.
 *  It's permissible to register multiple instances of this @ref bt_scan_cb
 *  type, in case different modules of an application are interested in
 *  tracking the scanning state. If a callback is not of interest for
 *  an instance, it may be set to NULL and will as a consequence not be
 *  used for that instance.
 */
struct bt_scan_cb {
	/**@brief Scan filter matched.
	 *
	 * @param[in] device_info Data needed to establish
	 *                        connection and advertising information.
	 * @param[in] filter_match Filter match status.
	 * @param[in] connectable Inform that device is connectable.
	 */
	void (*filter_match)(struct bt_scan_device_info *device_info,
			     struct bt_scan_filter_match *filter_match,
			     bool connectable);

	/**@brief Scan filter unmatched. The device was not found.
	 *
	 * @param[in] device_info Data needed to establish
	 *                        connection and advertising information.
	 * @param[in] connectable Inform that device is connectable.
	 *
	 * @note Even if the filters are disable and not set, then all devices
	 *       will be reported by this callback.
	 *       It can be useful if the scan is used without filters.
	 */
	void (*filter_no_match)(struct bt_scan_device_info *device_info,
				bool connectable);

	/**@brief Error when connecting.
	 *
	 * @param[in] device_info Data needed to establish
	 *                        connection and advertising information.
	 */
	void (*connecting_error)(struct bt_scan_device_info *device_info);
	/**@brief Connecting data.
	 *
	 * @param[in] device_info Data needed to establish
	 *                        connection and advertising information.
	 * @param[in] conn Connection object.
	 */
	void (*connecting)(struct bt_scan_device_info *device_info,
			   struct bt_conn *conn);

	sys_snode_t node;
};

/**@brief Register scanning callbacks.
 *
 * Register callbacks to monitor the state of scanning.
 *
 * @param cb Callback struct.
 */
void bt_scan_cb_register(struct bt_scan_cb *cb);


/**@brief Function for initializing the Scanning Module.
 *
 * @param[in] init Pointer to scanning module initialization structure.
 *                 Can be initialized as NULL. If NULL, the parameters
 *                 required to initialize the module are loaded from
 *                 static configuration. If module is to establish the
 *                 connection automatically, this must be initialized
 *                 with the relevant data.
 */
void bt_scan_init(const struct bt_scan_init_param *init);

/**@brief Function for starting scanning.
 *
 * @details This function starts the scanning according to
 *          the configuration set during the initialization.
 *
 * @param[in] scan_type The choice between active and passive scanning.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error
 *	   code is returned.
 */
int bt_scan_start(enum bt_scan_type scan_type);

/**@brief Function for stopping scanning.
 */
int bt_scan_stop(void);

#if CONFIG_BT_SCAN_FILTER_ENABLE

/**@brief Function for enabling filtering.
 *
 * @details The filters can be combined with each other.
 *          For example, you can enable one filter or several filters.
 *          For example, (BT_SCAN_NAME_FILTER | BT_SCAN_UUID_FILTER)
 *          enables UUID and name filters.
 *
 * @param[in] mode Filter mode: @ref BT_SCAN_FILTER_MODE.
 * @param[in] match_all If this flag is set, all types of enabled filters
 *                      must be matched before generating
 *                      @ref BT_SCAN_EVT_FILTER_MATCH to the main
 *                      application. Otherwise, it is enough to
 *                      match one filter to trigger the filter match event.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error
 *	     code is returned.
 *
 */
int bt_scan_filter_enable(u8_t mode, bool match_all);

/**@brief Function for disabling filtering.
 *
 * @details This function disables all filters.
 *          Even if the automatic connection establishing is enabled,
 *          the connection will not be established with the first
 *          device found after this function is called.
 */
void bt_scan_filter_disable(void);

/**@brief Function for getting filter status.
 *
 * @details This function returns the filter setting and whether
 *          it is enabled or disabled.

 * @param[out] status Pointer to Filter Status structure.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error
 *	     code is returned.
 */
int bt_scan_filter_status(struct bt_filter_status *status);

/**@brief Function for adding any type of filter to the scanning.
 *
 * @details This function adds a new filter by type.
 *          The filter will be added if
 *          there is available space in this filter type array, and
 *          if the same filter has not already been set.
 *
 * @param[in] type Filter type.
 * @param[in] data Pointer to The filter data to add.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error
 *	     code is returned.
 */
int bt_scan_filter_add(enum bt_scan_filter_type type,
		       const void *data);

/**@brief Function for removing all set filters.
 *
 * @details The function removes all previously set filters.
 *
 * @note After using this function the filters are still enabled.
 */
void bt_scan_filter_remove_all(void);

#endif /* CONFIG_BT_SCAN_FILTER_ENABLE */

/**@brief Function for changing the scanning parameters.
 *
 * @details Use this function to change scanning parameters.
 *          During the parameter change the scan is stopped.
 *          To resume scanning, use @ref bt_scan_start.
 *          Scanning parameters can be set to NULL. If so,
 *          the default static configuration is used.
 *
 * @param[in] scan_param Pointer to Scanning parameters.
 *                       Can be initialized as NULL.
 *
 * @return 0 If the operation was successful. Otherwise, a (negative) error
 *	     code is returned.
 */
int bt_scan_params_set(struct bt_le_scan_param *scan_param);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_SCAN_H_ */
