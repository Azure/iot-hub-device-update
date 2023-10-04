cmake_minimum_required (VERSION 3.5)

set (target_name diagnostics_workflow)

include (agentRules)

compileasc99 ()

add_library (${target_name} STATIC src/diagnostics_workflow.c src/diagnostics_result.c)
add_library (diagnostics_component::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc)

find_package (Parson REQUIRED)

target_link_aziotsharedutil (${target_name} PRIVATE)

if (NOT WIN32)
    target_link_libraries (${target_name} PRIVATE diagnostic_utils::file_upload_utility)
endif ()

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils diagnostics_component::diagnostics_async_helper
           diagnostic_utils::diagnostics_config_utils
    PRIVATE aduc::logging
            aduc::system_utils
            diagnostics_component::diagnostics_interface
            diagnostics_component::diagnostics_devicename
            diagnostic_utils::file_info_utils
            diagnostic_utils::operation_id_utils
            Parson::parson
            parson_json_utils)
