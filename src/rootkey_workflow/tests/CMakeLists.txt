set (target_name rootkey_workflow_unit_test)

include (agentRules)
compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${target_name})
target_sources (${target_name} PRIVATE rootkey_workflow_ut.cpp)

target_link_libraries (
    ${target_name}
    PRIVATE Catch2::Catch2WithMain
            aduc::c_utils
            aduc::rootkey_workflow
            aduc::root_key_utils)

include (CTest)
include (Catch)
if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests (${target_name})
else ()
    add_test (NAME ${target_name} COMMAND ${target_name})
endif ()