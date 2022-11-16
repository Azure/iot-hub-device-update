cmake_minimum_required (VERSION 3.5)

project (rootkeypackage_utils_unit_test)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources main.cpp rootkeypackage_utils_ut.cpp)

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_link_libraries (${PROJECT_NAME} PRIVATE
    aduc::rootkeypackage_utils
    Catch2::Catch2)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
