cmake_minimum_required (VERSION 3.5)

set (target_name diagnostics_workflow)

include (agentRules)

compileasc99 ()

add_library (${target_name} STATIC src/diagnostics_workflow.c src/diagnostics_result.c)
add_library (diagnostics_component::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc)

target_compile_definitions (
    ${target_name} PRIVATE DIAGNOSTICS_LOG_STAGING_FOLDER="${DIAGNOSTICS_LOG_STAGING_FOLDER}")
# NOTE: the call to find_package for azure_c_shared_utility
# must come before umqtt since their config.cmake files expect the aziotsharedutil target to already have been defined.
find_package (azure_c_shared_utility REQUIRED)
find_package (Parson REQUIRED)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils diagnostics_component::diagnostics_async_helper
    PRIVATE aziotsharedutil
            aduc::logging
            aduc::system_utils
            diagnostics_component::diagnostics_interface
            diagnostics_component::diagnostics_devicename
            diagnostic_utils::azure_blob_storage_utils
            diagnostic_utils::file_info_utils
            Parson::parson
            parson_json_utils)

if (ADUC_BUILD_UNIT_TESTS)
    find_package (umock_c REQUIRED CONFIG)
    target_link_libraries (${target_name} PRIVATE umock_c)
    add_subdirectory (tests)
endif ()
