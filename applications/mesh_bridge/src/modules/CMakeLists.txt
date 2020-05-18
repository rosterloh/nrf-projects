target_sources_ifdef(CONFIG_DEVICE_POWER_MANAGEMENT
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/power_handler.c)

#target_sources_ifdef(CONFIG_SERIAL
#		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/uart_handler.c)

target_sources_ifdef(CONFIG_BRIDGE_CDC_ENABLE
         app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/usb_cdc_handler.c)

target_sources_ifdef(CONFIG_NETWORKING
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/wifi_handler.c)
