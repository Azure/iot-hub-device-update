cmake_minimum_required (VERSION 3.5)

project (script_handler_unit_test)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources main.cpp script_handler_ut.cpp ../src/script_handler.cpp
             ../../../update_manifest_handlers/steps_handler/src/steps_handler.cpp)

add_executable (${PROJECT_NAME} ${sources})
target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)

# Windows needs all EXEs to have links to all potential libraries so this is included here
if (WIN32)
    target_link_dosdk (${PROJECT_NAME} PRIVATE)
endif ()

find_package (Catch2 REQUIRED)
find_package (IotHubClient REQUIRED)
find_package (umqtt REQUIRED)

target_include_directories (
    ${PROJECT_NAME}
    PUBLIC inc
    PRIVATE ${PROJECT_SOURCE_DIR}/inc
            ${PROJECT_SOURCE_DIR}/../inc
            ${ADUC_EXPORT_INCLUDES}
            ${SCRIPT_HANDLER_INCLUDES}
            ${ADU_EXTENSION_INCLUDES}
            ${ADU_SHELL_INCLUDES}
            ${PROJECT_SOURCE_DIR}/../../../update_manifest_handlers/steps_handler/inc)

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::adu_types
            aduc::config_utils
            aduc::contract_utils
            aduc::extension_manager
            aduc::parser_utils
            aduc::process_utils
            aduc::string_utils
            aduc::system_utils
            aduc::workflow_data_utils
            aduc::workflow_utils
            Catch2::Catch2)

target_link_libraries (${PROJECT_NAME} PRIVATE libaducpal)

target_compile_definitions (
    ${PROJECT_NAME} PRIVATE ADUSHELL_FILE_PATH="${ADUSHELL_FILE_PATH}"
                            ADUC_TEST_DATA_FOLDER="${ADUC_TEST_DATA_FOLDER}")

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
