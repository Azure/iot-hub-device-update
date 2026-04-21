cmake_minimum_required (VERSION 3.5)

project (operation_id_utils_ut)

include (agentRules)

compileasc99 ()
disablertti ()

# Compile the source directly into the test binary so the overridden
# DIAGNOSTICS_COMPLETED_OPERATION_FILE_PATH takes effect in the library code.
set (sources operation_id_utils_ut.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../src/operation_id_utils.c)

find_package (Catch2 REQUIRED)
find_package (Parson REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

# Use a temporary path for tests to avoid permission issues with production path
target_compile_definitions (
    ${PROJECT_NAME}
    PRIVATE DIAGNOSTICS_COMPLETED_OPERATION_FILE_PATH="/tmp/test_diagnostics_operation_id"
)

target_include_directories (${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../inc)

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::c_utils
            aduc::logging
            aduc::system_utils
            aduc::parson_json_utils
            diagnostics_component::diagnostics_interface
            libaducpal
            Parson::parson
            Catch2::Catch2WithMain)

include (CTest)
include (Catch)
if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests (${PROJECT_NAME})
else ()
    add_test (NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
endif ()
