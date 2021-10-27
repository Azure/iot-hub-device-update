cmake_minimum_required (VERSION 3.5)

project (workflow_persistence_utils_unit_test)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources main.cpp workflow_persistence_utils_ut.cpp)

find_package (Catch2 REQUIRED)
find_package (Parson REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_include_directories (
    ${PROJECT_NAME}
    PRIVATE ${WORKING_DIRECTORY}/../inc
)

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::c_utils
            aduc::logging
            aduc::workflow_persistence_utils
            Catch2::Catch2
            Parson::parson)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
