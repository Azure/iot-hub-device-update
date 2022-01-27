cmake_minimum_required (VERSION 3.5)

project (linux_platform_layer_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources main.cpp download_ut.cpp mock_do_download.cpp)

find_package (Catch2 REQUIRED)
find_package (azure_c_shared_utility REQUIRED)
find_package (IotHubClient REQUIRED)
find_package (umqtt REQUIRED)

add_executable (${PROJECT_NAME} ${sources})

target_include_directories (${PROJECT_NAME} PRIVATE ${ADUC_EXPORT_INCLUDES})

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC IotHubClient::iothub_client
           iothub_client_mqtt_transport umqtt

    PRIVATE aduc::linux_platform_layer
            aduc::logging
            aduc::system_utils
            aduc::exception_utils
            aduc::c_utils
            Catch2::Catch2)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
