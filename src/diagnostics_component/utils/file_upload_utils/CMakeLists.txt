cmake_minimum_required (VERSION 3.13)

include (agentRules)

set (target_name file_upload_utility)

add_library (${target_name} STATIC src/file_upload_utility.cpp src/blob_storage_helper.cpp
                                   src/blob_storage_helper.hpp)
add_library (diagnostic_utils::${target_name} ALIAS ${target_name})

target_link_aziotsharedutil (${target_name} PUBLIC)

find_package (azure-storage-blobs-cpp CONFIG REQUIRED)

include (agentRules)

include (find_curl_and_import_libcurl)

find_curl_and_import_libcurl ()

target_include_directories (${target_name} PUBLIC inc)

target_link_libraries (${target_name} PRIVATE aduc::c_utils aduc::exception_utils
                                              Azure::azure-storage-blobs CURL::libcurl)
