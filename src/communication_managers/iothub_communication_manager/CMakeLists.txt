cmake_minimum_required (VERSION 3.5)

project (iothub_communication_manager)

add_library (${PROJECT_NAME} STATIC src/iothub_communication_manager.c)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (${PROJECT_NAME} PUBLIC ./inc ${ADUC_TYPES_INCLUDES}
                                                   ${ADUC_EXPORT_INCLUDES})

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

find_package (azure_c_shared_utility REQUIRED)
find_package (umqtt REQUIRED)

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aduc::adu_types
    PRIVATE aduc::communication_abstraction
            aduc::config_utils
            aduc::c_utils
            aduc::eis_utils
            aduc::https_proxy_utils
            aduc::logging
            aduc::retry_utils)

            if (ADUC_IOT_HUB_PROTOCOL STREQUAL "MQTT")
            target_compile_definitions (${PROJECT_NAME} PRIVATE ADUC_ALLOW_MQTT=1)
            target_link_libraries (${PROJECT_NAME} PRIVATE iothub_client_mqtt_transport)

        elseif (ADUC_IOT_HUB_PROTOCOL STREQUAL "MQTT_over_WebSockets")
            target_compile_definitions (${PROJECT_NAME} PRIVATE ADUC_ALLOW_MQTT_OVER_WEBSOCKETS=1)
            target_link_libraries (${PROJECT_NAME} PRIVATE iothub_client_mqtt_ws_transport)

        elseif (ADUC_IOT_HUB_PROTOCOL STREQUAL "IotHub_Protocol_from_Config")
            target_compile_definitions (${PROJECT_NAME} PRIVATE ADUC_GET_IOTHUB_PROTOCOL_FROM_CONFIG=1)
            target_compile_definitions (${PROJECT_NAME} PRIVATE ADUC_ALLOW_MQTT=1)
            target_compile_definitions (${PROJECT_NAME} PRIVATE ADUC_ALLOW_MQTT_OVER_WEBSOCKETS=1)
            target_link_libraries (${PROJECT_NAME} PRIVATE iothub_client_mqtt_transport)
            target_link_libraries (${PROJECT_NAME} PRIVATE iothub_client_mqtt_ws_transport)

        else ()
            message (
                FATAL_ERROR,
                "Invalid value '${ADUC_IOT_HUB_PROTOCOL}' for ADUC_IOT_HUB_PROTOCOL. Valid values: MQTT | MQTT_over_WebSockets | IotHub_Protocol_from_Config"
            )
        endif ()

target_compile_definitions (
    ${PROJECT_NAME}
    PRIVATE ADUC_AGENT_FILEPATH="${ADUC_AGENT_FILEPATH}"
            ADUC_CONF_FILE_PATH="${ADUC_CONF_FILE_PATH}"
)
