cmake_minimum_required (VERSION 3.5)

project (swupdate_handler_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

set (
    sources
    main.cpp
    swupdate_handler_v2_ut.cpp
    ../src/handler_create.cpp
    ../src/swupdate_handler_v2.cpp
    ../../../update_manifest_handlers/steps_handler/src/steps_handler.cpp)

find_package (Catch2 REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_include_directories (
    ${PROJECT_NAME}
    PRIVATE ${ADUC_EXPORT_INCLUDES}
            ${ADU_EXTENSION_INCLUDES}
            ${ADU_SHELL_INCLUDES}
            ${PROJECT_SOURCE_DIR}/inc
            ${PROJECT_SOURCE_DIR}/../inc
            ${PROJECT_SOURCE_DIR}/../../inc
            ${PROJECT_SOURCE_DIR}/../../../update_manifest_handlers/steps_handler/inc)

target_compile_definitions (
    ${PROJECT_NAME}
    PRIVATE ADUC_SWUPDATE_HANDLER_CONF_FILE_PATH="${ADUC_SWUPDATE_HANDLER_CONF_FILE_PATH}")

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::contract_utils
            aduc::c_utils
            aduc::exception_utils
            aduc::extension_manager
            aduc::process_utils
            aduc::string_utils
            aduc::system_utils
            aduc::workflow_data_utils
            aduc::workflow_utils
            Catch2::Catch2)

# Ensure that ctest discovers catch2 tests.
# Use catch_discover_tests() rather than add_test()
# See https://github.com/catchorg/Catch2/blob/master/contrib/Catch.cmake
include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
