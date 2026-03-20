cmake_minimum_required (VERSION 3.5)

project (steps_handler_unit_tests)

include (agentRules)
compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)
find_package (Parson REQUIRED)

set (target_name steps_handler_unit_tests)

add_executable (
    ${target_name}
    steps_handler_ut.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/steps_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/handler_create.cpp)

target_include_directories (
    ${target_name}
    PRIVATE
        ${ADUC_EXPORT_INCLUDES}
        ${ADU_EXTENSION_INCLUDES}
        ${ADU_SHELL_INCLUDES}
        ${CMAKE_CURRENT_SOURCE_DIR}/../inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../extension_manager/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../logging/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../logging/zlog/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/c_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/config_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/contract_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/exception_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/extension_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/parser_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/string_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/system_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../utils/workflow_utils/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../download_handlers/download_handler_factory/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../download_handlers/download_handler_plugin/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../shared_lib/inc)

target_compile_definitions (
    ${target_name}
    PRIVATE
        ADUC_INSTALLEDCRITERIA_FILE_PATH="/${ADUC_TMP_DIR_PATH}/adu-test-installedcriteria"
        ADUC_USE_ZLOGGING=1
        ADUC_TEST_DATA_FOLDER="${ADUC_TEST_DATA_FOLDER}")

target_link_libraries (
    ${target_name}
    PRIVATE
        aduc::agent_workflow
        aduc::adu_types
        aduc::contract_utils
        aduc::c_utils
        aduc::exception_utils
        aduc::extension_manager
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

target_link_aziotsharedutil (${target_name} PRIVATE)
target_link_libraries (${target_name} PRIVATE libaducpal)

include (CTest)
include (Catch)
catch_discover_tests (${target_name})
