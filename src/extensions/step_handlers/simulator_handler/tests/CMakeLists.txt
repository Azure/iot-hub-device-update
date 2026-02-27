cmake_minimum_required (VERSION 3.5)

project (simulator_handler_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources simulator_handler_unit_tests.cpp ../src/simulator_handler.cpp)

find_package (Catch2 REQUIRED)
find_package (IotHubClient REQUIRED)
find_package (umqtt REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_include_directories (
    ${PROJECT_NAME}
    PRIVATE ${ADUC_EXPORT_INCLUDES} ${ADU_EXTENSION_INCLUDES} ${ADU_SHELL_INCLUDES}
            ${PROJECT_SOURCE_DIR}/inc ${PROJECT_SOURCE_DIR}/../inc ${PROJECT_SOURCE_DIR}/../../inc)

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
            Parson::parson
            Catch2::Catch2WithMain)

# Windows needs all EXEs to have links to all potential libraries so this is included here
if (WIN32)
    target_link_dosdk (${PROJECT_NAME} PRIVATE)
endif ()

target_link_libraries (${PROJECT_NAME} PRIVATE libaducpal)

# Ensure that ctest discovers catch2 tests.
# Use catch_discover_tests() rather than add_test()
# See https://github.com/catchorg/Catch2/blob/master/contrib/Catch.cmake
include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})

# =====================================================================
# Mock-based test target for simulator_handler.cpp coverage
# =====================================================================

set (simulator_handler_mock_test_target simulator_handler_mock_unit_tests)

add_executable (
    ${simulator_handler_mock_test_target}
    simulator_handler_mock_ut.cpp
    mock_simulator_handler_deps.cpp
    ../src/simulator_handler.cpp)

target_include_directories (
    ${simulator_handler_mock_test_target}
    PRIVATE ${ADUC_EXPORT_INCLUDES}
            ${ADU_EXTENSION_INCLUDES}
            ${ADU_SHELL_INCLUDES}
            ${PROJECT_SOURCE_DIR}/inc
            ${PROJECT_SOURCE_DIR}/../../inc
            ${PROJECT_SOURCE_DIR}/../inc
            # logging
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../../logging/inc
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../../logging/zlog/inc
            # utility headers
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/parser_utils/inc
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/workflow_utils/inc)

target_compile_definitions (
    ${simulator_handler_mock_test_target}
    PRIVATE ADUC_USE_ZLOGGING=1)

find_package (Parson REQUIRED)

target_link_libraries (
    ${simulator_handler_mock_test_target}
    PRIVATE aduc::contract_utils
            Parson::parson
            Catch2::Catch2WithMain)

target_link_libraries (${simulator_handler_mock_test_target} PRIVATE libaducpal)

catch_discover_tests (${simulator_handler_mock_test_target})
