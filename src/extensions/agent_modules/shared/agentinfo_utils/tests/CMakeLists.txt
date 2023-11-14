set (target_name agentinfo_utils_unit_tests)

include (agentRules)
compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${target_name} src/main.cpp src/adu_agentinfo_utils_unit_tests.cpp)
target_link_libraries (${target_name} PRIVATE Catch2::Catch2
                                              aduc::agentinfo_utils
                                              aduc::agent_state_store
                                              aduc::mqtt_broker_common)
include (CTest)
include (Catch)
catch_discover_tests (${target_name})
