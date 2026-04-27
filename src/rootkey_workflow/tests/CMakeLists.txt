set (target_name rootkey_workflow_unit_test)

include (agentRules)
compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${target_name})
target_sources (${target_name} PRIVATE rootkey_workflow_ut.cpp)

if (ADUC_BUILD_UNIT_TESTS)
    set (ROOTKEY_WORKFLOW_TEST_STORE_PATH "${CMAKE_BINARY_DIR}/rootkey_workflow_test_store")
    set (ROOTKEY_WORKFLOW_TEST_STORE_PACKAGE_PATH "${ROOTKEY_WORKFLOW_TEST_STORE_PATH}/rootkeys.json")
endif ()

target_link_libraries (
    ${target_name}
    PRIVATE Catch2::Catch2WithMain
            aduc::c_utils
            aduc::rootkey_workflow
            aduc::root_key_utils)

if (ADUC_BUILD_UNIT_TESTS)
    target_compile_definitions (
        ${target_name}
        PRIVATE ADUC_ROOTKEY_STORE_PATH="${ROOTKEY_WORKFLOW_TEST_STORE_PATH}"
                ADUC_ROOTKEY_STORE_PACKAGE_PATH="${ROOTKEY_WORKFLOW_TEST_STORE_PACKAGE_PATH}")
endif ()

include (CTest)
include (Catch)
if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests (${target_name})
else ()
    add_test (NAME ${target_name} COMMAND ${target_name})
endif ()