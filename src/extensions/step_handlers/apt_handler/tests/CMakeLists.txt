cmake_minimum_required (VERSION 3.5)

project (apt_handler_unit_tests)

if (NOT ${ADUC_PLATFORM_LAYER} STREQUAL "linux")
    message (FATAL_ERROR "${PROJECT_NAME} requires the linux platform layer.")
endif ()

include (agentRules)

compileasc99 ()
disablertti ()

set (sources main.cpp apt_parser_ut.cpp ../src/apt_handler.cpp ../src/apt_parser.cpp)

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
            Catch2::Catch2)

target_link_libraries (${PROJECT_NAME} PRIVATE libaducpal)

# Ensure that ctest discovers catch2 tests.
# Use catch_discover_tests() rather than add_test()
# See https://github.com/catchorg/Catch2/blob/master/contrib/Catch.cmake
include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
