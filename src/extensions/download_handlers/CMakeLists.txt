add_subdirectory (download_handler_factory)
add_subdirectory (download_handler_plugin)

if (ADUC_BUILD_SNAP)
    message(STATUS "Skipping plugin_examples build for snap")
else ()
    add_subdirectory (plugin_examples)
endif ()
