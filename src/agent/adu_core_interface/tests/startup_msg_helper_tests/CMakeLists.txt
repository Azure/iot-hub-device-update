cmake_minimum_required (VERSION 3.5)

project (startup_msg_helper_unit_test)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)
find_package (Parson REQUIRED)

add_executable (${PROJECT_NAME} startup_msg_helper_ut.cpp
                                ${CMAKE_SOURCE_DIR}/src/agent/adu_core_interface/src/startup_msg_helper.c)

target_include_directories (
    ${PROJECT_NAME}
    PRIVATE ${CMAKE_SOURCE_DIR}/src/agent/adu_core_interface/src
            ${CMAKE_SOURCE_DIR}/src/agent/adu_core_interface/inc
            ${CMAKE_SOURCE_DIR}/src/utils/config_utils/inc
            ${ADUC_EXPORT_INCLUDES})

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE Catch2::Catch2WithMain
            Parson::parson
            aduc::c_utils
            aduc::string_utils
            aduc::adu_types)

target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)

target_compile_definitions (
    ${PROJECT_NAME}
    PRIVATE ENABLE_ADU_TELEMETRY_REPORTING
            ADUC_AGENT_VERSION="test"
            ADUC_BUILDER_IDENTIFIER="test-builder")

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
