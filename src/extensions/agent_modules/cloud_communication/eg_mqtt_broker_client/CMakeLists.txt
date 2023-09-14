cmake_minimum_required (VERSION 3.5)

set (target_name microsoft_du_mqttbroker_client)

# # The property name in du-config.json that contains all configurations
# # for the MQTT broker communication.
# set (DU_CLIENT_MQTT_CONF_PROP_NAME "mqttBrokerCommunication")

# set (DU_CONFIG_FILE_NAME "du-config.json")

add_library (${target_name} STATIC)

add_library (aduc::${target_name} ALIAS ${target_name})

find_package (Parson REQUIRED)

target_sources (${target_name} PRIVATE
    src/adu_agent_info_management.c
    src/adu_communication_channel.c
    src/adu_enrollment_management.c
    src/adu_mosquitto_utils.c
    src/adu_mqtt_client_module.c
    src/adu_updates_management.c
)

target_include_directories (
    ${target_name}
    PUBLIC inc
    PRIVATE ${PROJECT_SOURCE_DIR}/inc
            ./inc ../inc ../../inc
            ${ADU_AGENT_SDK_INCLUDES}
            )

target_link_libraries (
    ${target_name}
    PRIVATE
        aduc::adu_types
        aduc::mosquitto_client_extension_c
        aduc::config_utils
        aduc::logging
    #   aduc::workflow_data_utils
    #   aduc::workflow_utils
        Parson::parson)

# target_compile_definitions (
#     ${target_name}
#     PRIVATE ADUC_LOG_FOLDER="${ADUC_LOG_FOLDER}"
#             ADUC_SWUPDATE_HANDLER_CONF_FILE_PATH="${ADUC_SWUPDATE_HANDLER_CONF_FILE_PATH}"
#             ADUC_VERSION_FILE="${ADUC_VERSION_FILE}")

install (TARGETS ${target_name} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
