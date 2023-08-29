cmake_minimum_required (VERSION 3.13)

set(target_name file_upload_utility)

add_library(${target_name} STATIC src/file_upload_utility.cpp src/blob_storage_helper.cpp src/blob_storage_helper.hpp )
add_library (diagnostic_utils::${target_name} ALIAS ${target_name})

find_package (azure_c_shared_utility REQUIRED)
find_package(azure-storage-blobs-cpp CONFIG REQUIRED)

include (agentRules)

include (find_curl_and_import_libcurl)

find_curl_and_import_libcurl ()

target_include_directories (${target_name} PUBLIC inc)

target_link_libraries (
    ${target_name}
    PUBLIC aziotsharedutil
    PRIVATE  Azure::azure-storage-blobs Azure::azure-core CURL::libcurl)
