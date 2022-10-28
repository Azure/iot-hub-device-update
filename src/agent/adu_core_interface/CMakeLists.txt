project (adu_core_interface)

include (agentRules)
compileasc99 ()

find_package (azure_c_shared_utility REQUIRED)
find_package (IotHubClient REQUIRED)
find_package (Parson REQUIRED)

add_library (${PROJECT_NAME} STATIC "")
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${PROJECT_NAME} PUBLIC inc ${ADUC_EXPORT_INCLUDES})

target_sources (${PROJECT_NAME} PRIVATE src/adu_core_interface.c src/device_properties.c
                                        src/startup_msg_helper.c)

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aduc::adu_types
           aduc::c_utils
           aduc::communication_abstraction
           IotHubClient::iothub_client
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
            aduc::workflow_data_utils
            aduc::workflow_utils
            aziotsharedutil
            Parson::parson)

target_compile_definitions (
    ${PROJECT_NAME}
    PRIVATE ADUC_DEVICEPROPERTIES_MANUFACTURER="${ADUC_DEVICEPROPERTIES_MANUFACTURER}"
            ADUC_CONF_FILE_PATH="${ADUC_CONF_FILE_PATH}"
            ADUC_DEVICEPROPERTIES_MODEL="${ADUC_DEVICEPROPERTIES_MODEL}"
            ADUC_VERSION="${ADUC_VERSION}"
            ADUC_BUILDER_IDENTIFIER="${ADUC_BUILDER_IDENTIFIER}")

if (NOT
    ADUC_PLATFORM_LAYER
    STREQUAL
    "simulator")
    find_package (deliveryoptimization_sdk CONFIG REQUIRED)
    target_link_libraries (${PROJECT_NAME} PRIVATE Microsoft::deliveryoptimization)
else ()
    target_compile_definitions (${PROJECT_NAME} PRIVATE ADUC_PLATFORM_SIMULATOR)
endif ()

if (ENABLE_ADU_TELEMETRY_REPORTING)
    target_compile_definitions (${PROJECT_NAME} PRIVATE ENABLE_ADU_TELEMETRY_REPORTING)
endif ()
