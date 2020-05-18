target_sources(app PRIVATE
		     ${CMAKE_CURRENT_SOURCE_DIR}/module_state_event.c
		     ${CMAKE_CURRENT_SOURCE_DIR}/cdc_data_event.c
		     ${CMAKE_CURRENT_SOURCE_DIR}/uart_data_event.c
		     ${CMAKE_CURRENT_SOURCE_DIR}/power_event.c)
