target_sources(app PRIVATE
	       ${CMAKE_CURRENT_SOURCE_DIR}/battery_event.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/button_event.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/module_state_event.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/power_event.c
)

target_sources_ifdef(CONFIG_MESH_SENSOR_LED_ENABLE app
      PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/led_event.c)

target_sources_ifdef(CONFIG_SENSOR app
			PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/sensor_event.c)
