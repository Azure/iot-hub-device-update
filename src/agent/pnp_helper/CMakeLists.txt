project (pnp_helper)

include (agentRules)

compileasc99 ()

add_library (${PROJECT_NAME} STATIC ./src/pnp_protocol.c )
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (
    ${PROJECT_NAME}
    PUBLIC inc
    PRIVATE ${ADUC_EXPORT_INCLUDES})

target_link_digital_twin_client (${PROJECT_NAME} PUBLIC)

target_link_libraries (${PROJECT_NAME} PRIVATE IotHubClient::iothub_client
                                               aduc::communication_abstraction
                                               iothub_client_mqtt_transport umqtt)
