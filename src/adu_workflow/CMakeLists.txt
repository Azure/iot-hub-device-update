project (agent_workflow)

include (agentRules)

compileasc99 ()

add_library (${PROJECT_NAME} STATIC "")

set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_sources (${PROJECT_NAME} PRIVATE src/agent_workflow.c)

target_include_directories (${PROJECT_NAME} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

find_package (Parson REQUIRED)

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aduc::adu_types
    PRIVATE aduc::adu_core_export_helpers
            aduc::agent_orchestration
            aduc::c_utils
            aduc::download_handler_factory
            aduc::download_handler_plugin
            aduc::logging
            aduc::parser_utils
            aduc::system_utils
            aduc::workflow_data_utils
            aduc::workflow_utils)

target_compile_definitions (${PROJECT_NAME} PRIVATE ADUC_CONF_FILE_PATH="${ADUC_CONF_FILE_PATH}")

