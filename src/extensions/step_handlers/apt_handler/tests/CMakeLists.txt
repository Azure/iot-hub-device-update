cmake_minimum_required (VERSION 3.5)

project (apt_handler_unit_tests)

if (NOT ${ADUC_PLATFORM_LAYER} STREQUAL "linux")
    message (FATAL_ERROR "${PROJECT_NAME} requires the linux platform layer.")
endif ()

include (agentRules)

compileasc99 ()
disablertti ()

set (sources apt_parser_ut.cpp ../src/apt_handler.cpp ../src/apt_parser.cpp)

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_include_directories (
    ${PROJECT_NAME}
    PRIVATE ${ADUC_EXPORT_INCLUDES} ${ADU_EXTENSION_INCLUDES} ${ADU_SHELL_INCLUDES}
            ${PROJECT_SOURCE_DIR}/inc ${PROJECT_SOURCE_DIR}/../../inc ${PROJECT_SOURCE_DIR}/../inc)

target_compile_definitions (
    ${PROJECT_NAME}
    PRIVATE ADUC_INSTALLEDCRITERIA_FILE_PATH="/${ADUC_TMP_DIR_PATH}/adu-test-installedcriteria"
            ADUSHELL_FILE_PATH="${ADUSHELL_FILE_PATH}")

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::agent_orchestration
            aduc::config_utils
            aduc::contract_utils
            aduc::exception_utils
            aduc::extension_manager
            aduc::installed_criteria_utils
            aduc::parser_utils
            aduc::process_utils
            aduc::string_utils
            aduc::system_utils
            aduc::workflow_data_utils
            aduc::workflow_utils
            Catch2::Catch2WithMain)

target_link_libraries (${PROJECT_NAME} PRIVATE libaducpal)

# Ensure that ctest discovers catch2 tests.
# Use catch_discover_tests() rather than add_test()
# See https://github.com/catchorg/Catch2/blob/master/contrib/Catch.cmake
include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})

# =====================================================================
# Mock-based test target for apt_handler.cpp coverage
# =====================================================================

set (apt_handler_mock_test_target apt_handler_mock_unit_tests)

add_executable (
    ${apt_handler_mock_test_target}
    apt_handler_mock_ut.cpp
    mock_apt_handler_deps.cpp
    ../src/apt_handler.cpp
    ../src/apt_parser.cpp)

target_include_directories (
    ${apt_handler_mock_test_target}
    PRIVATE ${ADUC_EXPORT_INCLUDES}
            ${ADU_EXTENSION_INCLUDES}
            ${ADU_SHELL_INCLUDES}
            ${PROJECT_SOURCE_DIR}/inc
            ${PROJECT_SOURCE_DIR}/../../inc
            ${PROJECT_SOURCE_DIR}/../inc
            # extension_manager headers
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../extension_manager/inc
            # logging
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../../logging/inc
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../../logging/zlog/inc
            # utility headers
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/config_utils/inc
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/installed_criteria_utils/inc
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/parser_utils/inc
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/process_utils/inc
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/string_utils/inc
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/system_utils/inc
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/workflow_data_utils/inc
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/workflow_utils/inc
            # download handler headers (for extension_manager.hpp dependency chain)
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../download_handlers/download_handler_factory/inc
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../download_handlers/download_handler_plugin/inc
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../shared_lib/inc)

target_compile_definitions (
    ${apt_handler_mock_test_target}
    PRIVATE ADUC_INSTALLEDCRITERIA_FILE_PATH="/${ADUC_TMP_DIR_PATH}/adu-test-installedcriteria"
            ADUSHELL_FILE_PATH="${ADUSHELL_FILE_PATH}"
            ADUC_USE_ZLOGGING=1)

find_package (Parson REQUIRED)

target_link_libraries (
    ${apt_handler_mock_test_target}
    PRIVATE aduc::contract_utils
            Parson::parson
            Catch2::Catch2WithMain)

target_link_libraries (${apt_handler_mock_test_target} PRIVATE libaducpal)

catch_discover_tests (${apt_handler_mock_test_target})
