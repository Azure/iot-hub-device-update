cmake_minimum_required (VERSION 3.5)

set (target_name diagnostics_workflow)

include (agentRules)

compileasc99 ()

add_library (${target_name} STATIC src/diagnostics_workflow.c src/diagnostics_result.c)
add_library (diagnostics_component::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc)

# NOTE: the call to find_package for azure_c_shared_utility
# must come before umqtt since their config.cmake files expect the aziotsharedutil target to already have been defined.
find_package (azure_c_shared_utility REQUIRED)
find_package (azure-storage-blobs-cpp CONFIG REQUIRED)
find_package (Parson REQUIRED)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils diagnostics_component::diagnostics_async_helper
           diagnostic_utils::diagnostics_config_utils
    PRIVATE aduc::logging
            aduc::system_utils
            aziotsharedutil
            azure_blob_storage_file_upload_utility
            Azure::azure-storage-blobs
            diagnostics_component::diagnostics_interface
            diagnostics_component::diagnostics_devicename
            diagnostic_utils::file_info_utils
            diagnostic_utils::operation_id_utils
            Parson::parson
            parson_json_utils)
