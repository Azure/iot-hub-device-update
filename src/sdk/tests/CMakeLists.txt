set (target_name aducsdk_unit_tests)

include (agentRules)
compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

# Per-test data subdirectory under the project-wide test data folder, matching
# the convention used elsewhere in the repo (e.g. apisvc_unit_tests).
set (SDK_TEST_DATA_DIR "${ADUC_TEST_DATA_FOLDER}/aducsdk")

add_executable (${target_name})

# Compile aducsdk.c privately so we can override its hard-coded production
# paths to point at a writable test directory. We deliberately do NOT link
# aduc::sdk here because that target is built with the production paths.
target_sources (
    ${target_name} PRIVATE aducsdk_unit_tests.cpp
                           ${CMAKE_SOURCE_DIR}/src/sdk/src/aducsdk.c)

target_include_directories (${target_name} PRIVATE ${CMAKE_SOURCE_DIR}/src/sdk/inc)

target_compile_definitions (
    ${target_name}
    PRIVATE ADUC_API_DEFAULT_FIFO_PATH="${SDK_TEST_DATA_DIR}/api/apireq.fifo"
            ADUC_DATA_FOLDER="${SDK_TEST_DATA_DIR}"
            ADUC_TEST_DATA_FOLDER="${ADUC_TEST_DATA_FOLDER}")

target_link_libraries (
    ${target_name}
    PRIVATE Catch2::Catch2WithMain
            aduc::apiproto_utils
            aduc::apisvc
            aduc::c_utils
            aduc::viewstatemgr)

include (CTest)
include (Catch)
if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests (${target_name})
else ()
    add_test (NAME ${target_name} COMMAND ${target_name})
endif ()
