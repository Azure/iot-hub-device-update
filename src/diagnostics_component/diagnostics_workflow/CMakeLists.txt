cmake_minimum_required (VERSION 3.5)

set (target_name diagnostics_workflow)

include (agentRules)

compileasc99 ()

add_library (${target_name} STATIC src/diagnostics_workflow.c src/diagnostics_result.c)
add_library (diagnostics_component::${target_name} ALIAS ${target_name})

target_include_directories (${target_name} PUBLIC inc)

find_package (azure-storage-blobs-cpp CONFIG REQUIRED)
find_package (Parson REQUIRED)

target_link_aziotsharedutil (${target_name} PRIVATE)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils diagnostics_component::diagnostics_async_helper
           diagnostic_utils::diagnostics_config_utils
    PRIVATE aduc::logging
            aduc::system_utils
            Azure::azure-storage-blobs
            diagnostics_component::diagnostics_interface
            diagnostics_component::diagnostics_devicename
            diagnostic_utils::file_info_utils
            diagnostic_utils::operation_id_utils
            Parson::parson
            parson_json_utils)

if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    # TODO(JeffMill): [PAL] AzureBlobStorageFileUploadUtility_UploadFilesToContainer
    # https://github.com/Azure/azure-blob-storage-file-upload-utility
else ()
    target_link_libraries (${target_name} PRIVATE azure_blob_storage_file_upload_utility)
endif ()
