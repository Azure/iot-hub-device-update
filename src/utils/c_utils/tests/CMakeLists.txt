cmake_minimum_required (VERSION 3.5)

project (c_utils_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources main.cpp c_utils_ut.cpp connection_string_utils_ut.cpp)

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_link_libraries (${PROJECT_NAME} PRIVATE aduc::c_utils aduc::string_utils Catch2::Catch2)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
