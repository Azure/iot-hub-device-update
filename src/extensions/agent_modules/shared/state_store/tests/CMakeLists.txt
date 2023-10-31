cmake_minimum_required (VERSION 3.5)

set (target_name agent_state_store_unit_tests)

include (agentRules)

disablertti ()

set (sources main.cpp agent_state_store_ut.cpp)

find_package (Catch2 REQUIRED)
find_package (Parson REQUIRED)


add_executable (${target_name} ${sources})

target_link_libraries (${target_name} PRIVATE aduc::agent_state_store
                                              aduc::c_utils
                                              Parson::parson
                                              Catch2::Catch2)

target_link_umock_c (${target_name} PRIVATE)

include (CTest)
include (Catch)
catch_discover_tests (${target_name})
