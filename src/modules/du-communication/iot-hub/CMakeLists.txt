cmake_minimum_required (VERSION 3.5)

set (target_name microsoft_du_communication_iothub)

# The property name in du-config.json that contains all configurations
# for the MQTT broker communication.
set (DU_CLIENT_MQTT_CONF_PROP_NAME "iotHubCommunication")

set (DU_CONFIG_FILE_NAME "du-config.json")

add_library (${target_name} MODULE)

add_library (aduc::${target_name} ALIAS ${target_name})

# NOTE: the call to find_package for azure_c_shared_utility
# must come before umqtt since their config.cmake files expect the aziotsharedutil target to already have been defined.
find_package (azure_c_shared_utility REQUIRED)
find_package (IotHubClient REQUIRED)
find_package (umqtt REQUIRED)
find_package (Parson REQUIRED)

target_sources (${target_name} PRIVATE src/iothub-communication-module.c)

target_include_directories (
    ${target_name}
    PUBLIC inc
    PRIVATE ${PROJECT_SOURCE_DIR}/inc
            ${ADUC_SDK_INCLUDES}
            )

target_link_libraries (
    ${target_name}
    PRIVATE
        aziotsharedutil
        IotHubClient::iothub_client
        umqtt
        aduc::adu_core_interface
        aduc::c_utils
        aduc::config_utils
        aduc::command_helper
        aduc::communication_abstraction
        aduc::d2c_messaging
        aduc::device_info_interface
        aduc::extension_manager
        aduc::iothub_communication_manager
        aduc::logging
        aduc::pnp_helper
        Parson::parson
        # TODO (nox-mfst) - enable diagnostics feature
        # diagnostics_component::diagnostics_interface
        # diagnostics_component::diagnostics_devicename
        )

if (ADUC_IOT_HUB_PROTOCOL STREQUAL "MQTT")
    target_compile_definitions (${target_name} PRIVATE ADUC_ALLOW_MQTT=1)
    target_link_libraries (${target_name} PRIVATE iothub_client_mqtt_transport)

elseif (ADUC_IOT_HUB_PROTOCOL STREQUAL "MQTT_over_WebSockets")
    target_compile_definitions (${target_name} PRIVATE ADUC_ALLOW_MQTT_OVER_WEBSOCKETS=1)
    target_link_libraries (${target_name} PRIVATE iothub_client_mqtt_ws_transport)

elseif (ADUC_IOT_HUB_PROTOCOL STREQUAL "IotHub_Protocol_from_Config")
    target_compile_definitions (${target_name} PRIVATE ADUC_GET_IOTHUB_PROTOCOL_FROM_CONFIG=1)
    target_compile_definitions (${target_name} PRIVATE ADUC_ALLOW_MQTT=1)
    target_compile_definitions (${target_name} PRIVATE ADUC_ALLOW_MQTT_OVER_WEBSOCKETS=1)
    target_link_libraries (${target_name} PRIVATE iothub_client_mqtt_transport)
    target_link_libraries (${target_name} PRIVATE iothub_client_mqtt_ws_transport)

else ()
    message (
        FATAL_ERROR,
        "Invalid value '${ADUC_IOT_HUB_PROTOCOL}' for ADUC_IOT_HUB_PROTOCOL. Valid values: MQTT | MQTT_over_WebSockets | IotHub_Protocol_from_Config"
    )
endif ()

install (TARGETS ${target_name} LIBRARY DESTINATION ${ADUC_EXTENSIONS_INSTALL_FOLDER})

add_subdirectory (adu_core_interface)
add_subdirectory (device_info_interface)
add_subdirectory (pnp_helper)

# TODO (nox-msft) - enable unit tests
# if (ADUC_BUILD_UNIT_TESTS)

#     add_subdirectory (tests)

# endif ()
