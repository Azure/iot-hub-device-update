project (plugin_examples)

if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    # TODO(JeffMill): Getting LINK : fatal error LNK1104: cannot open file 'azure_macro_utils_c.lib' for microsoft_delta_download_handler
    message (NOTICE "[Windows] Not building microsoft_delta_download_handler")
else ()
    add_subdirectory (microsoft_delta_download_handler)
endif ()
