project (microsoft_delta_download_handler_util_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

# ---------------------------------------------------------------
# Existing integration-style tests (use real workflow_utils)
# ---------------------------------------------------------------
add_executable (${PROJECT_NAME} "")

target_sources (${PROJECT_NAME} PRIVATE microsoft_delta_download_handler_utils_ut.cpp)

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::adu_types
            aduc::microsoft_delta_download_handler_utils
            aduc::parser_utils
            aduc::workflow_utils
            Catch2::Catch2WithMain)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})

# ---------------------------------------------------------------
# Mock-based tests for utils.c (all functions, mocked deps)
# ---------------------------------------------------------------
set (utils_mock_test_target microsoft_delta_download_handler_utils_mock_unit_tests)

add_executable (${utils_mock_test_target} "")

target_sources (
    ${utils_mock_test_target}
    PRIVATE
        microsoft_delta_download_handler_utils_mock_ut.cpp
        mock_utils_deps.c
        # Compile utils.c directly with mocked deps
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/microsoft_delta_download_handler_utils.c)

target_include_directories (
    ${utils_mock_test_target}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../inc
        ${ADUC_EXPORT_INCLUDES}
        ${ADU_EXTENSION_INCLUDES}
        # Headers needed by utils.c (functions are mocked via link-time substitution)
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../source_update_cache/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../extension_manager/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../utils/workflow_utils/inc)

target_link_aziotsharedutil (${utils_mock_test_target} PRIVATE)

target_link_libraries (
    ${utils_mock_test_target}
    PRIVATE aduc::adu_types
            aduc::c_utils
            aduc::logging
            Catch2::Catch2WithMain)

target_link_libraries (${utils_mock_test_target} PRIVATE libaducpal)

catch_discover_tests (${utils_mock_test_target})

# ---------------------------------------------------------------
# Mock-based tests for utils.cpp (ProcessDeltaUpdate, mock SharedLib + diff API)
# ---------------------------------------------------------------
set (utils_cpp_test_target microsoft_delta_download_handler_utils_cpp_unit_tests)

add_executable (${utils_cpp_test_target} "")

target_sources (
    ${utils_cpp_test_target}
    PRIVATE
        microsoft_delta_download_handler_utils_cpp_ut.cpp
        mock_diff_api.cpp
        # Compile utils.cpp directly with mock SharedLib
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/microsoft_delta_download_handler_utils.cpp)

target_include_directories (
    ${utils_cpp_test_target}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../inc
        ${ADUC_EXPORT_INCLUDES}
        ${ADU_EXTENSION_INCLUDES}
        # shared_lib.hpp is in this include path
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../shared_lib/inc)

target_link_libraries (
    ${utils_cpp_test_target}
    PRIVATE aduc::adu_types
            aduc::c_utils
            aduc::logging
            Catch2::Catch2WithMain)

target_link_libraries (${utils_cpp_test_target} PRIVATE libaducpal)

catch_discover_tests (${utils_cpp_test_target})
