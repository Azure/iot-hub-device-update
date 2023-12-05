set (target_name microsoft_du_mqttbroker_client)

include (agentRules)
compileasc99 ()
disablertti ()

add_library (${target_name} STATIC)

add_library (aduc::${target_name} ALIAS ${target_name})

find_package (azure_c_shared_utility REQUIRED)
find_package (Parson REQUIRED)

target_sources (
    ${target_name}
    PRIVATE src/adu_agent_info_management.c
            src/adu_agent_info_management_internal.c
            src/adu_enrollment_management.c
            src/adu_enrollment_management_internal.c
            src/adu_update_management.c
            src/adu_update_management_internal.c
            src/adu_mqtt_client_module.c
            src/adu_integration.cpp
            src/adu_processupdate.cpp
            src/agentinfo_request_operation.c
            src/enrollment_request_operation.c
            src/upd_request_operation.c
            src/eg_mqtt_broker_client.c
            src/topic_mgmt_lifecycle.c)

target_include_directories (${target_name} PUBLIC inc ${ADUC_EXPORT_INCLUDES})
target_include_directories (${target_name} PRIVATE ${ADU_EXTENSION_INCLUDES})

target_link_libraries (
    ${target_name}
    PUBLIC aduc::agent_state_store
           aduc::c_utils
           aduc::communication_channel_mqtt_broker
           aduc::mqtt_broker_common
           aduc::retry_utils
           aduc::adu_types
           Parson::parson
    PRIVATE
            aduc::agentinfo_utils
            aduc::enrollment_utils
            aduc::extension_manager
            aduc::upd_utils
            aduc::config_utils
            aduc::contract_utils
            aduc::logging
            aduc::mosquitto_utils
            aduc::parser_utils
            aduc::download_handler_plugin
            aduc::download_handler_factory
            aduc::workflow_data_utils
            aduc::parson_json_utils
            aduc::string_utils
            aduc::system_utils
            aduc::workqueue
            aduc::workflow_utils
            aziotsharedutil)

install (TARGETS ${target_name} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})
