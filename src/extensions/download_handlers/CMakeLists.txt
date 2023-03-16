project (download_handlers)

add_subdirectory (download_handler_factory)
add_subdirectory (download_handler_plugin)

if (NOT ADUC_BUILD_SNAP)
    add_subdirectory (plugin_examples)
endif()
