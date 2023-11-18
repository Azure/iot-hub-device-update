
set (target_name adu_mqtt_common_unit_tests)

include (agentRules)
compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${target_name} adu_mqtt_common_unit_tests.cpp)
target_link_libraries (${target_name} PRIVATE Catch2::Catch2
                                              aduc::mqtt_broker_common
                                              aduc::agent_state_store
                                              aduc::system_utils)
include (CTest)
include (Catch)
catch_discover_tests (${target_name})
