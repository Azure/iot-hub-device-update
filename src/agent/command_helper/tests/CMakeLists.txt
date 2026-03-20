cmake_minimum_required (VERSION 3.5)

project (command_helper_unit_test)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME} command_helper_ut.cpp
                                ${CMAKE_SOURCE_DIR}/src/agent/command_helper/src/command_helper.c)

target_include_directories (
    ${PROJECT_NAME}
    PRIVATE ${CMAKE_SOURCE_DIR}/src/agent/command_helper/inc
            ${CMAKE_SOURCE_DIR}/src/utils/permission_utils/inc
            ${ADUC_EXPORT_INCLUDES})

target_compile_definitions (
    ${PROJECT_NAME}
    PRIVATE ADUC_COMMANDS_FIFO_NAME="/tmp/test-aduc-commands.fifo"
            ADUC_FILE_GROUP="adu"
            ADUC_FILE_USER="adu")

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE Catch2::Catch2WithMain
            aduc::c_utils
            aduc::adu_types
            aduc::string_utils
            libaducpal
            pthread)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
