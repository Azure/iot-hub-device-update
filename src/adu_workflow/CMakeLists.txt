project (agent_workflow)

include (agentRules)

compileasc99 ()

add_library (
    ${PROJECT_NAME} STATIC
    src/agent_workflow.c)

set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC inc ${ADUC_EXPORT_INCLUDES})
target_link_digital_twin_client (${PROJECT_NAME} PUBLIC)

find_package (Parson REQUIRED)

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aduc::adu_types
    PRIVATE 
            aduc::logging
            aduc::parser_utils
            aduc::workflow_data_utils
            aduc::workflow_utils
            Parson::parson
            -zdef)

target_compile_definitions (
    ${PROJECT_NAME}
    PRIVATE ADUC_CONF_FILE_PATH="${ADUC_CONF_FILE_PATH}"
            ADUC_BUILD_UNIT_TESTS="${ADUC_BUILD_UNIT_TESTS}")

