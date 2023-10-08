cmake_minimum_required (VERSION 3.5)

set (target_name microsoft_du_mqttbroker_client)

add_library (${target_name} STATIC)

add_library (aduc::${target_name} ALIAS ${target_name})

find_package (azure_c_shared_utility REQUIRED)
find_package (Parson REQUIRED)

target_sources (${target_name} PRIVATE src/adu_enrollment_management.c src/adu_mqtt_client_module.c
                                       src/adu_updates_management.c src/eg_mqtt_broker_client.c)

target_include_directories (
    ${target_name}
    PUBLIC inc
    PRIVATE ${PROJECT_SOURCE_DIR}/inc ./inc ../inc ../../inc ${ADU_AGENT_SDK_INCLUDES})

target_link_libraries (
    ${target_name}
    PUBLIC aduc::communication_channel_mqtt_broker aduc::agent_state_store
    PRIVATE aduc::adu_types
            aduc::config_utils
            aduc::logging
            aduc::mosquitto_utils
            aduc::parson_json_utils
            aduc::retry_utils
            Parson::parson
            aziotsharedutil)

install (TARGETS ${target_name} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})

add_subdirectory (tests)
