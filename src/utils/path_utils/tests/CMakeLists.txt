cmake_minimum_required (VERSION 3.5)

project (path_utils_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME} "")

target_sources (${PROJECT_NAME} PRIVATE main.cpp path_utils_ut.cpp)

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)

target_link_libraries (${PROJECT_NAME} PRIVATE aduc::path_utils aduc::string_utils Catch2::Catch2)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
