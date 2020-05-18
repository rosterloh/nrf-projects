target_sources_ifdef(CONFIG_SETTINGS
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/settings_loader.c)

target_sources_ifdef(CONFIG_NW_SNS_USB_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/usb_state.c)
