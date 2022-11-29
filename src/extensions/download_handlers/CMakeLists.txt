project (download_handlers)

add_subdirectory (download_handler_factory)
add_subdirectory (download_handler_plugin)

if (NOT ADUC_UBUNTU_CORE_SNAP_ONLY)
    add_subdirectory (plugin_examples)
endif()
