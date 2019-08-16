# Point to root directory.
set(RO_DIR ${CMAKE_CURRENT_LIST_DIR} CACHE PATH "RO root directory")

zephyr_include_directories(include)

add_subdirectory(subsys)
add_subdirectory(drivers)
