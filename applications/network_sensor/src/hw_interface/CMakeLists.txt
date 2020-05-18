target_sources_ifdef(CONFIG_NW_SNS_BUTTONS_ENABLE app
		     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/buttons.c)

target_sources_ifdef(CONFIG_NW_SNS_LED_ENABLE app
		     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/leds.c)

target_sources_ifdef(CONFIG_DISPLAY app
		     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/display.c)
