set (target_name apisvc_unit_tests)

include (agentRules)
compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

# SDK uses the same per-test data dir as the rest of this test (apisvc subdir).
set (APISVC_TEST_DATA_DIR "${ADUC_TEST_DATA_FOLDER}/apisvc")

add_executable (${target_name})
target_sources (${target_name} PRIVATE apisvc_unit_tests.cpp)

# Compile aducsdk.c privately so its hard-coded production paths can be
# overridden to point at a writable test directory (matches the path the
# test passes to init_api_svc).
target_sources (${target_name} PRIVATE ${CMAKE_SOURCE_DIR}/src/sdk/src/aducsdk.c)
target_include_directories (${target_name} PRIVATE ${CMAKE_SOURCE_DIR}/src/sdk/inc)

target_compile_definitions (
    ${target_name}
    PRIVATE ADUC_API_DEFAULT_FIFO_PATH="${APISVC_TEST_DATA_DIR}/test_req_fifo"
            ADUC_DATA_FOLDER="${APISVC_TEST_DATA_DIR}"
            ADUC_FILE_GROUP="${ADUC_FILE_GROUP}"
            ADUC_FILE_USER="${ADUC_FILE_USER}"
            ADUC_TEST_DATA_FOLDER="${ADUC_TEST_DATA_FOLDER}")

target_link_libraries (
    ${target_name}
    PRIVATE Catch2::Catch2WithMain
            aduc::apiproto_utils
            aduc::apisvc
            aduc::auto_utils
            aduc::config_utils
            aduc::logging
            aduc::viewstatemgr)

include (CTest)
include (Catch)
if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests (${target_name})
else ()
    add_test (NAME ${target_name} COMMAND ${target_name})
endif ()
