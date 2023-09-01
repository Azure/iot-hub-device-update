set (target_name device_info_interface)

include (agentRules)

compileasc99 ()

add_library (${target_name} STATIC src/device_info_interface.c)
add_library (aduc::${target_name} ALIAS ${target_name})

target_include_directories (
    ${target_name}
    PUBLIC inc
    PRIVATE ${ADUC_EXPORT_INCLUDES})

target_link_iothub_client (${target_name} PUBLIC)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils aduc::communication_abstraction
    PRIVATE aduc::d2c_messaging
            aduc::logging
            aduc::pnp_helper
            IotHubClient::iothub_client)

target_link_libraries (${target_name} PRIVATE libaducpal)
