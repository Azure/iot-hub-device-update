project (plugin_examples)

if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    message (NOTICE "[Windows] Not building microsoft_delta_download_handler")
else ()
    add_subdirectory (microsoft_delta_download_handler)
endif ()
