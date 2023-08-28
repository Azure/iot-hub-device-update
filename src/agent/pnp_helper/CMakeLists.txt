set (target_name pnp_helper)
include (agentRules)

compileasc99 ()

add_library (${target_name} STATIC ./src/pnp_protocol.c)
add_library (aduc::${target_name} ALIAS ${target_name})
#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (
    ${target_name}
    PUBLIC inc
    PRIVATE ${ADUC_EXPORT_INCLUDES})

target_link_iothub_client (${target_name} PUBLIC)

target_link_libraries (
    ${target_name}
    PRIVATE IotHubClient::iothub_client
            aduc::communication_abstraction
            aduc::logging
            iothub_client_mqtt_transport
            umqtt)
