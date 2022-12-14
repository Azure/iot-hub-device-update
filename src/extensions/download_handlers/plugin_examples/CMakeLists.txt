project (plugin_examples)

if (_WIN32)
    message (STATUS "[Windows] Not building microsoft_delta_download_handler")
else ()
    add_subdirectory (microsoft_delta_download_handler)
endif ()
