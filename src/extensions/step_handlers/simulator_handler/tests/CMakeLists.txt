cmake_minimum_required (VERSION 3.5)

project (simulator_handler_unit_tests)

if (NOT
    ${ADUC_PLATFORM_LAYER}
    STREQUAL
    "linux")
    message (FATAL_ERROR "${PROJECT_NAME} requires the linux platform layer.")
endif ()

include (agentRules)

compileasc99 ()
disablertti ()

set (sources main.cpp simulator_handler_unit_tests.cpp ../src/simulator_handler.cpp)

find_package (Catch2 REQUIRED)
find_package (azure_c_shared_utility REQUIRED)
find_package (IotHubClient REQUIRED)
find_package (umqtt REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_include_directories (
    ${PROJECT_NAME}
    PRIVATE ${ADUC_EXPORT_INCLUDES}
            ${ADU_EXTENSION_INCLUDES}
            ${ADU_SHELL_INCLUDES}
            ${PROJECT_SOURCE_DIR}/inc
            ${PROJECT_SOURCE_DIR}/../inc
            ${PROJECT_SOURCE_DIR}/../../inc)

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aduc::extension_manager
    PRIVATE aduc::contract_utils
            aduc::exception_utils
            aduc::extension_utils
            aduc::logging
            aduc::parser_utils
            aduc::process_utils
            aduc::string_utils
            aduc::system_utils
            aduc::workflow_data_utils
            aduc::workflow_utils
            Parson::parson)

# Ensure that ctest discovers catch2 tests.
# Use catch_discover_tests() rather than add_test()
# See https://github.com/catchorg/Catch2/blob/master/contrib/Catch.cmake
include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
