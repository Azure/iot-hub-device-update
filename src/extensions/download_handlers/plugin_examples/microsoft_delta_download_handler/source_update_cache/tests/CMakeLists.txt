project (source_update_cache_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)
find_package (Parson REQUIRED)

# ---------------------------------------------------------------
# Existing integration-style tests for source_update_cache_utils
# ---------------------------------------------------------------
add_executable (${PROJECT_NAME} "")

target_include_directories (${PROJECT_NAME} PRIVATE inc ${ADUC_EXPORT_INCLUDES})

target_sources (${PROJECT_NAME} PRIVATE source_update_cache_utils_ut.cpp)

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::file_utils
            aduc::source_update_cache
            aduc::test_utils
            aduc::system_utils
            aduc::workflow_utils
            Catch2::Catch2WithMain
            Parson::parson)

target_link_libraries (${PROJECT_NAME} PRIVATE libaducpal)

target_compile_definitions (
    ${PROJECT_NAME}
    PRIVATE
        ADUC_DELTA_DOWNLOAD_HANDLER_SOURCE_UPDATE_CACHE_DIR="${ADUC_DELTA_DOWNLOAD_HANDLER_SOURCE_UPDATE_CACHE_DIR}"
)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})

# ---------------------------------------------------------------
# Mock-based tests for source_update_cache.c (Lookup + Move)
# ---------------------------------------------------------------
set (suc_mock_test_target source_update_cache_mock_unit_tests)

add_executable (${suc_mock_test_target} "")

target_sources (
    ${suc_mock_test_target}
    PRIVATE
        source_update_cache_ut.cpp
        mock_source_update_cache_deps.c
        # Compile source_update_cache.c directly so we can intercept deps
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/source_update_cache.c)

target_include_directories (
    ${suc_mock_test_target}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../inc
        ${ADUC_EXPORT_INCLUDES}
        # Headers needed by source_update_cache.c (functions are mocked)
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../utils/permission_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../utils/system_utils/inc)

target_link_aziotsharedutil (${suc_mock_test_target} PRIVATE)

target_link_libraries (
    ${suc_mock_test_target}
    PRIVATE aduc::adu_types
            aduc::c_utils
            aduc::logging
            Catch2::Catch2WithMain)

target_link_libraries (${suc_mock_test_target} PRIVATE libaducpal)

catch_discover_tests (${suc_mock_test_target})
