set (target_name enrollment_utils_unit_tests)

include (agentRules)
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${target_name} src/main.cpp src/adu_enrollment_utils_unit_tests.cpp)
target_link_libraries (${target_name} PRIVATE Catch2::Catch2
                                              aduc::enrollment_utils
                                              aduc::agent_state_store
                                              aduc::system_utils)
include (CTest)
include (Catch)
catch_discover_tests (${target_name})
