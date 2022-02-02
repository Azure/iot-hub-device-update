cmake_minimum_required (VERSION 3.5)

project (script_handler_unit_test)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources main.cpp script_handler_ut.cpp ../src/script_handler.cpp)

find_package (Catch2 REQUIRED)
find_package (azure_c_shared_utility REQUIRED)
find_package (IotHubClient REQUIRED)
find_package (umqtt REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_include_directories (
    ${PROJECT_NAME}
    PUBLIC inc
           ${ADUC_EXPORT_INCLUDES}
           ${SCRIPT_HANDLER_INCLUDES}
           ${ADU_EXTENSION_INCLUDES}
           ${ADU_SHELL_INCLUDES})

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::adu_types
            aduc::extension_manager
            aduc::parser_utils
            aduc::process_utils
            aduc::string_utils
            aduc::workflow_data_utils
            aduc::workflow_utils
            Catch2::Catch2
            aziotsharedutil)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
