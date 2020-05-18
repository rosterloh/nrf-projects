target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_state.c)

target_sources_ifdef(CONFIG_SETTINGS
         app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/settings_loader.c)

target_sources_ifdef(CONFIG_MESH_SENSOR_LED_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/led_state.c)

target_sources_ifdef(CONFIG_MESH_SENSOR_POWER_MANAGER_ENABLE
         app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/power_manager.c)

target_sources_ifdef(CONFIG_MESH_SENSOR_SHELL_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/shell.c)
