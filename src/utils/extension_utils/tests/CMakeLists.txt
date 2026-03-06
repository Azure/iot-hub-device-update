cmake_minimum_required (VERSION 3.5)

project (extension_utils_unit_test)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources extension_utils_ut.cpp)

find_package (Catch2 REQUIRED)
find_package (Parson REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE Catch2::Catch2WithMain Parson::parson)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
