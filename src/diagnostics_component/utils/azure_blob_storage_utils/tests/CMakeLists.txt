cmake_minimum_required (VERSION 3.5)

project (azure_blob_storage_ut)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources main.cpp azure_blob_storage_ut.cpp)

find_package (Catch2 REQUIRED)
find_package (azure_c_shared_utility REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::string_utils
            azure-storage-lite
            diagnostic_utils::azure_blob_storage_utils
            Catch2::Catch2
            aziotsharedutil)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
