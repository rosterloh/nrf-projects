cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(mesh_bridge)

target_sources(app PRIVATE src/main.c)

zephyr_library_include_directories(
  src/events
)

add_subdirectory(src/events)
add_subdirectory(src/modules)
