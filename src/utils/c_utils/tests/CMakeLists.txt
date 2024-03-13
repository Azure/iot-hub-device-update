cmake_minimum_required (VERSION 3.5)

set (target_name c_utils_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources main.cpp c_utils_ut.cpp connection_string_utils_ut.cpp)

find_package (Catch2 REQUIRED)

add_executable (${target_name} ${sources})

target_link_libraries (${target_name} PRIVATE aduc::c_utils aduc::string_utils aduc::system_utils
                                              Catch2::Catch2)

target_link_umock_c (${target_name} PRIVATE)

include (CTest)
include (Catch)
catch_discover_tests (${target_name})
