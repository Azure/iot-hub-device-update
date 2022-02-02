project (device_info_interface)

include (agentRules)

compileasc99 ()

add_library (${PROJECT_NAME} STATIC src/device_info_interface.c)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories (
    ${PROJECT_NAME}
    PUBLIC inc
    PRIVATE ${ADUC_EXPORT_INCLUDES})

target_link_digital_twin_client (${PROJECT_NAME} PUBLIC)

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC aduc::c_utils aduc::communication_abstraction
    PRIVATE aduc::d2c_messaging
            aduc::logging
            aduc::pnp_helper
            IotHubClient::iothub_client)
