target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/board.c)

target_sources_ifdef(CONFIG_MESH_SENSOR_BUTTONS_ENABLE app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/buttons.c)

target_sources_ifdef(CONFIG_MESH_SENSOR_LED_ENABLE app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/leds.c)

target_sources_ifdef(CONFIG_MESH_SENSOR_BATTERY_CHARGER_DISCRETE app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/battery_charger.c)

target_sources_ifdef(CONFIG_MESH_SENSOR_BATTERY_MEAS app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/battery_meas.c)
