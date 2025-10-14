set (target_name apisvc_unit_tests)

include (agentRules)
compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${target_name})
target_sources (${target_name} PRIVATE apisvc_unit_tests.cpp)

target_compile_definitions (
    ${target_name} PRIVATE ADUC_API_DEFAULT_FIFO_PATH="${ADUC_API_DEFAULT_FIFO_PATH}"
                           ADUC_FILE_GROUP="${ADUC_FILE_GROUP}"
                           ADUC_FILE_USER="${ADUC_FILE_USER}")

target_link_libraries (
    ${target_name}
    PRIVATE Catch2::Catch2WithMain
            aduc::apiproto_utils
            aduc::apisvc
            aduc::auto_utils
            aduc::config_utils
            aduc::logging
            aduc::sdk
            aduc::viewstatemgr)

include (CTest)
include (Catch)
catch_discover_tests (${target_name})
