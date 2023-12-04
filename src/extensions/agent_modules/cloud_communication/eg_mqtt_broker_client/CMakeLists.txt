set (target_name microsoft_du_mqttbroker_client)

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
            src/agentinfo_request_operation.c
            src/enrollment_request_operation.c
            src/upd_request_operation.c
            src/eg_mqtt_broker_client.c
            src/topic_mgmt_lifecycle.c)

target_include_directories (${target_name} PUBLIC inc)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::agent_state_store aduc::c_utils aduc::communication_channel_mqtt_broker
           aduc::mosquitto_utils aduc::mqtt_broker_common aduc::retry_utils
    PRIVATE aduc::adu_types
            aduc::agentinfo_utils
            aduc::enrollment_utils
            aduc::upd_utils
            aduc::config_utils
            aduc::logging
            aduc::parson_json_utils
            aduc::workqueue
            aziotsharedutil
            Parson::parson)

install (TARGETS ${target_name} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})
