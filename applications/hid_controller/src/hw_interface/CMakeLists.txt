target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/board.c)

# target_sources_ifdef(CONFIG_CONTROLLER_MOTION_SENSOR_ENABLE app PRIVATE
#                      ${CMAKE_CURRENT_SOURCE_DIR}/motion_sensor.c)

target_sources_ifdef(CONFIG_CONTROLLER_BUTTONS_ENABLE app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/buttons.c)

target_sources_ifdef(CONFIG_CONTROLLER_SELECTOR_HW_ENABLE app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/selector_hw.c)

target_sources_ifdef(CONFIG_CONTROLLER_LED_ENABLE app
		     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/leds.c)

# target_sources_ifdef(CONFIG_CONTROLLER_BATTERY_CHARGER_DISCRETE app
#                      PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/battery_charger.c)

target_sources_ifdef(CONFIG_CONTROLLER_BATTERY_MEAS app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/battery_meas.c)
