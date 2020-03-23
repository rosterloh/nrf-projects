target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/board.c)

target_sources_ifdef(CONFIG_CONTROLLER_BUTTONS_ENABLE app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/buttons.c)
# target_sources_ifdef(CONFIG_CONTROLLER_CLICK_DETECTOR_ENABLE app
#                      PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/click_detector.c)
target_sources_ifdef(CONFIG_CONTROLLER_PASSKEY_BUTTONS app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/passkey_buttons.c)

target_sources_ifdef(CONFIG_CONTROLLER_SELECTOR_HW_ENABLE app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/selector_hw.c)
# target_sources_ifdef(CONFIG_CONTROLLER_WHEEL_ENABLE app
#                      PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/wheel.c)

target_sources_ifdef(CONFIG_CONTROLLER_LED_ENABLE app
		     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/leds.c)

# target_sources_ifdef(CONFIG_CONTROLLER_BATTERY_CHARGER_DISCRETE app
#                      PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/battery_charger.c)

target_sources_ifdef(CONFIG_CONTROLLER_BATTERY_MEAS app
                     PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/battery_meas.c)
