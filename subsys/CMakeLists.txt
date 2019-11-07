if (CONFIG_BT)
  add_subdirectory(bluetooth)
endif()

add_subdirectory_ifdef(CONFIG_EVENT_MANAGER     event_manager)
add_subdirectory_ifdef(CONFIG_PROFILER          profiler)
add_subdirectory_ifdef(CONFIG_FW_METADATA       fw_metadata)
add_subdirectory_ifdef(CONFIG_SHELL_BT_NUS      shell)

add_subdirectory(debug)
add_subdirectory(partition_manager)
