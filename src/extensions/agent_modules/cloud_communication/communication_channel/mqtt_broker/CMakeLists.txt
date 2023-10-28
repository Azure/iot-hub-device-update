cmake_minimum_required (VERSION 3.5)

set (target_name communication_channel_mqtt_broker)

# # The property name in du-config.json that contains all configurations
# # for the MQTT broker communication.
# set (DU_CLIENT_MQTT_CONF_PROP_NAME "mqttBrokerCommunication")

# set (DU_CONFIG_FILE_NAME "du-config.json")

add_library (${target_name} STATIC)

add_library (aduc::${target_name} ALIAS ${target_name})

find_package (azure_c_shared_utility REQUIRED)
find_package (Parson REQUIRED)

target_sources (${target_name} PRIVATE src/adu_communication_channel.c)

target_include_directories (${target_name} PUBLIC inc)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils
           aduc::agent_state_store
           aduc::du_agent_sdk
           aduc::retry_utils
           aziotsharedutil

    PRIVATE aduc::adu_types
            aduc::config_utils
            aduc::logging
            aduc::mosquitto_utils
            aduc::d2c_messaging
            Parson::parson)
