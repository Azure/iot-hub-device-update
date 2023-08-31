set (target_name rootkey_workflow_unit_test)

include (agentRules)
compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${target_name})
target_sources (${target_name} PRIVATE main.cpp rootkey_workflow_ut.cpp)

target_link_libraries (
    ${target_name}
    PRIVATE aduc::c_utils
            aduc::logging
            aduc::rootkey_workflow
            aduc::root_key_utils
            Catch2::Catch2
            Parson::parson)

target_compile_definitions (${target_name}
                            PRIVATE ADUC_TEST_DATA_FOLDER="${ADUC_TEST_DATA_FOLDER}")

include (CTest)
include (Catch)
catch_discover_tests (${target_name})
