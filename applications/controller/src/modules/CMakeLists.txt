# target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_state.c)

# target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_bond.c)

# target_sources_ifdef(CONFIG_CONTROLLER_BLE_ADVERTISING_ENABLE
# 		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_adv.c)

# target_sources_ifdef(CONFIG_CONTROLLER_BLE_DISCOVERY_ENABLE
# 		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_discovery.c)

# target_sources_ifdef(CONFIG_CONTROLLER_BLE_SCANNING_ENABLE
# 		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_scan.c)

# target_sources_ifdef(CONFIG_CONTROLLER_CONFIG_ENABLE
# 		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/config.c)

# target_sources_ifdef(CONFIG_CONTROLLER_BLE_SCANNING_ENABLE
# 		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hid_forward.c)

# target_sources_ifdef(CONFIG_CONTROLLER_HID_STATE_ENABLE
# 		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hid_state.c)

# target_sources_ifdef(CONFIG_CONTROLLER_USB_ENABLE
# 		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/usb_state.c)

# target_sources_ifdef(CONFIG_CONTROLLER_LED_ENABLE
# 		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/led_state.c)

# target_sources_ifdef(CONFIG_CONTROLLER_LED_STREAM_ENABLE
# 		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/led_stream.c)

# target_sources_ifdef(CONFIG_CONTROLLER_POWER_MANAGER_ENABLE
# 		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/power_manager.c)

# target_sources_ifdef(CONFIG_CONTROLLER_SHELL_ENABLE
# 		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/shell.c)

# target_sources_ifdef(CONFIG_CONTROLLER_CONFIG_CHANNEL_DFU_ENABLE
# 		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dfu.c)

# target_sources_ifdef(CONFIG_CONTROLLER_FN_KEYS_ENABLE
# 		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/fn_keys.c)

# target_sources_ifdef(CONFIG_CONTROLLER_WATCHDOG_ENABLE
# 		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/watchdog.c)

# target_sources_ifdef(CONFIG_CONTROLLER_BAS_ENABLE
# 		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/bas.c)

# target_sources_ifdef(CONFIG_CONTROLLER_HIDS_ENABLE
# 		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hids.c)