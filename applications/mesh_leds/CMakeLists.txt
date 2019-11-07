cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project("Mesh LEDs"
        VERSION 0.1)

target_sources(app PRIVATE
	src/main.c
	src/prov_handler.c
	src/model_handler.c)
target_include_directories(app PRIVATE include)