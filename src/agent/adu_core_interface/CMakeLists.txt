set (target_name adu_core_interface)

include (agentRules)

compileasc99 ()

find_package (IotHubClient REQUIRED)
find_package (Parson REQUIRED)

add_library (${target_name} STATIC "")
add_library (aduc::${target_name} ALIAS ${target_name})

set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_sources (${target_name} PRIVATE src/adu_core_interface.c src/device_properties.c
                                       src/startup_msg_helper.c)

target_link_aziotsharedutil (${target_name} PRIVATE)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::adu_types aduc::c_utils aduc::communication_abstraction IotHubClient::iothub_client
    PRIVATE aduc::adu_core_export_helpers
            aduc::agent_orchestration
            aduc::agent_workflow
            aduc::config_utils
            aduc::d2c_messaging
            aduc::extension_manager
            aduc::hash_utils
            aduc::logging
            aduc::parser_utils
            aduc::pnp_helper
            aduc::reporting_utils
            aduc::rootkeypackage_utils
            aduc::rootkey_workflow
            aduc::workflow_data_utils
            aduc::workflow_utils
            Parson::parson)

target_link_libraries (${target_name} PRIVATE libaducpal)

target_compile_definitions (
    ${target_name}
    PRIVATE ADUC_DEVICEPROPERTIES_MANUFACTURER="${ADUC_DEVICEPROPERTIES_MANUFACTURER}"
            ADUC_CONF_FILE_PATH="${ADUC_CONF_FILE_PATH}"
            ADUC_DEVICEPROPERTIES_MODEL="${ADUC_DEVICEPROPERTIES_MODEL}"
            ADUC_VERSION="${ADUC_VERSION}"
            ADUC_BUILDER_IDENTIFIER="${ADUC_BUILDER_IDENTIFIER}")

if (ENABLE_ADU_TELEMETRY_REPORTING)
    target_compile_definitions (${target_name} PRIVATE ENABLE_ADU_TELEMETRY_REPORTING)
endif ()
