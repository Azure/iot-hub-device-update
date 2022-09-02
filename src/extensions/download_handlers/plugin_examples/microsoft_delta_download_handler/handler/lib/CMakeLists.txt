set (target_name microsoft-delta-download-handler)

include (agentRules)
compileasc99 ()

add_library (${target_name} STATIC)
add_library (aduc::${target_name} ALIAS ${target_name})

# Turn -fPIC on, in order to use this library in a shared library.
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_sources (${target_name} PRIVATE src/microsoft_delta_download_handler.c)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::adu_types
    PRIVATE aduc::c_utils
            aduc::microsoft_delta_download_handler_utils
            aduc::logging
            aduc::source_update_cache
            aduc::workflow_utils)
