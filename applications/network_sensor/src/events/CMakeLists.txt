target_sources(app PRIVATE
	       ${CMAKE_CURRENT_SOURCE_DIR}/button_event.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/module_state_event.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/power_event.c
)

target_sources_ifdef(CONFIG_NW_SNS_LED_ENABLE app
		     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/led_event.c)

target_sources_ifdef(CONFIG_NW_SNS_CONFIG_ENABLE app
		     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/config_event.c)

target_sources_ifdef(CONFIG_NW_SNS_USB_ENABLE app
		     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/usb_event.c)
