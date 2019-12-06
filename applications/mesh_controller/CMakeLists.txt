cmake_minimum_required(VERSION 3.13.1)

set(RO_SUPPORTED_BOARDS
  nrf52_adafruit_feather
  )

set(RO_SUPPORTED_BUILD_TYPES
  ZDebug
  ZRelease
  )

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE ZDebug)
endif()

include(../../cmake/boilerplate.cmake)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/dts.overlay")
  set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/dts.overlay")
endif()

set(mcuboot_CONF_FILE
  ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/mcuboot_${CMAKE_BUILD_TYPE}.conf
  )

# Define configuration files.
set(CONF_FILE "configuration/${BOARD}/app_${CMAKE_BUILD_TYPE}.conf")

################################################################################

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project("Mesh Controller"
        VERSION 0.1)

################################################################################

target_sources(app PRIVATE src/main.c)

# Include application events and configuration headers
zephyr_library_include_directories(
  src/events
  configuration/common
  configuration/${BOARD}
  )

# Application sources
add_subdirectory(src/events)
add_subdirectory(src/hw_interface)
add_subdirectory(src/modules)

if(CONFIG_BOOTLOADER_MCUBOOT)
  assert_exists(mcuboot_CONF_FILE)
endif()
