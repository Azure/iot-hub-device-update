project (d2c_messaging_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME})

target_sources (${PROJECT_NAME} PRIVATE d2c_messaging_ut.cpp)

target_include_directories (${PROJECT_NAME} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::communication_abstraction
            aduc::d2c_messaging
            aduc::retry_utils
            Catch2::Catch2WithMain)

include (CTest)
# NOTE: The pre-existing [functional] cases in d2c_messaging_ut.cpp were
# intentionally disabled in PR #738 (April 2025) because they hang in CI
# pipelines (Debian / Ubuntu amd64 hit the 1500s ctest timeout). They are
# still compiled into the binary and can be run manually for diagnostics:
#     d2c_messaging_unit_tests [functional]
# but they are NOT registered with ctest until they are stabilized.
#
# ADO Bug 38069154: the deterministic, single-threaded handle-refresh
# tests are safe for CI and ARE registered below.
add_test (NAME ${PROJECT_NAME}_handle_refresh COMMAND ${PROJECT_NAME} [handle_refresh])
