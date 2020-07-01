# Point to root directory.
set(RO_DIR ${CMAKE_CURRENT_LIST_DIR} CACHE PATH "RO root directory")

include(cmake/extensions.cmake)
include(cmake/multi_image.cmake)
include(cmake/reports.cmake)

zephyr_include_directories(include)

add_subdirectory(lib)
add_subdirectory(subsys)
add_subdirectory(drivers)
