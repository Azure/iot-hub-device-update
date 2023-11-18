set (target_name agent_workflow)

include (agentRules)

compileasc99 ()

add_library (${target_name} STATIC "")

add_library (aduc::${target_name} ALIAS ${target_name})

set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_sources (${target_name} PRIVATE src/agent_workflow.c)

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

find_package (Parson REQUIRED)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::adu_types
    PRIVATE aduc::adu_core_export_helpers
            aduc::agent_orchestration
            aduc::c_utils
            aduc::config_utils
            aduc::download_handler_factory
            aduc::download_handler_plugin
            aduc::logging
            aduc::parser_utils
            aduc::root_key_utils
            aduc::system_utils
            aduc::workflow_data_utils
            aduc::workflow_utils)

target_compile_definitions (${target_name} PRIVATE ADUC_CONF_FILE_PATH="${ADUC_CONF_FILE_PATH}")

target_link_libraries (${target_name} PRIVATE libaducpal)
target_link_aziotsharedutil (${target_name} PRIVATE)
