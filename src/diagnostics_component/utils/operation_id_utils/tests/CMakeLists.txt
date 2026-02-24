cmake_minimum_required (VERSION 3.5)

project (operation_id_utils_ut)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources operation_id_utils_ut.cpp)

find_package (Catch2 REQUIRED)
find_package (Parson REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

# Use a temporary path for tests to avoid permission issues with production path
target_compile_definitions (
    ${PROJECT_NAME}
    PRIVATE DIAGNOSTICS_COMPLETED_OPERATION_FILE_PATH="/tmp/test_diagnostics_operation_id"
)

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE diagnostic_utils::operation_id_utils
            aduc::system_utils
            aduc::parson_json_utils
            Parson::parson
            Catch2::Catch2WithMain)

include (CTest)
include (Catch)
if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests (${PROJECT_NAME})
else ()
    add_test (NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
endif ()