set (target_name rootkeypackage_test_app)

include (agentRules)

compileasc99 ()
disablertti ()

add_executable (${target_name})
target_sources (${target_name} PRIVATE src/main.cpp)

target_link_aziotsharedutil (${target_name} INTERFACE)
target_link_libraries (${target_name} PRIVATE aduc::rootkeypackage_utils aduc::test_utils)
