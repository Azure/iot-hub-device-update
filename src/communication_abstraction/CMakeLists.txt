cmake_minimum_required (VERSION 3.5)

include (agentRules)

set (target_name communication_abstraction)

add_library (${target_name} STATIC src/client_handle_helper.c)

add_library (aduc::${target_name} ALIAS ${target_name})
#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${target_name} PUBLIC inc)

# NOTE: the call to find_package for azure_c_shared_utility
# must come before umqtt since their config.cmake files expect the aziotsharedutil target to already have been defined.
target_link_aziotsharedutil (${target_name} PRIVATE)
find_package (IotHubClient REQUIRED)
find_package (umqtt REQUIRED)

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils aduc::adu_types
    PRIVATE aduc::adu_types IotHubClient::iothub_client iothub_client_mqtt_transport umqtt uhttp
            aduc::logging)

if (WIN32)
    # prov_auth_client, hsm_security_client, uhttp are required when
    # azure-iot-sdk-c is compiled using -Duse_edge_modules=ON
    target_link_libraries (${target_name} PRIVATE prov_auth_client hsm_security_client)
endif ()
