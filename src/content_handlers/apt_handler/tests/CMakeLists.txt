cmake_minimum_required (VERSION 3.5)

project (apt_handler_unit_tests)

if (NOT
    ${ADUC_PLATFORM_LAYER}
    STREQUAL
    "linux")
    message (FATAL_ERROR "${PROJECT_NAME} requires the linux platform layer.")
endif ()

include (agentRules)

compileasc99 ()
disablertti ()

set (sources main.cpp apt_handler_ut.cpp apt_parser_ut.cpp)

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_include_directories (
    ${PROJECT_NAME}
    PRIVATE ${ADUC_EXPORT_INCLUDES}
            ${ADU_SHELL_INCLUDES}
            ${PROJECT_SOURCE_DIR}/inc
            ${PROJECT_SOURCE_DIR}/../../inc
            ${PROJECT_SOURCE_DIR}/../inc)

target_compile_definitions (
    ${PROJECT_NAME} PRIVATE ADUC_INSTALLEDCRITERIA_FILE_PATH="/tmp/adu-test-installedcriteria")

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::adu_core_interface
            aduc::apt_handler
            aduc::exception_utils
            aduc::string_utils
            aduc::system_utils
            Catch2::Catch2)

# Ensure that ctest discovers catch2 tests.
# Use catch_discover_tests() rather than add_test()
# See https://github.com/catchorg/Catch2/blob/master/contrib/Catch.cmake
include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})