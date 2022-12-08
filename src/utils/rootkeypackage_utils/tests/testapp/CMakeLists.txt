set (target_name rootkeypackage_test_app)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (azure_c_shared_utility REQUIRED)

add_executable (${target_name})
target_sources (${target_name} PRIVATE src/main.cpp)

target_link_libraries (${target_name} PRIVATE aduc::rootkeypackage_utils aduc::test_utils
                                              aziotsharedutil)
