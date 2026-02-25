cmake_minimum_required (VERSION 3.5)

set (target_name adu_types_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources adu_types_ut.cpp)

find_package (Catch2 REQUIRED)

add_executable (${target_name} ${sources})

target_link_libraries (
    ${target_name} PRIVATE aduc::adu_types aduc::c_utils Catch2::Catch2WithMain)

include (CTest)
include (Catch)
catch_discover_tests (${target_name})
