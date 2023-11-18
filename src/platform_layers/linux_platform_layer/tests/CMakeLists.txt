cmake_minimum_required (VERSION 3.5)

project (linux_platform_layer_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

set (sources main.cpp download_ut.cpp mock_do_download.cpp)

add_executable (${PROJECT_NAME} ${sources})

find_package (Catch2 REQUIRED)
find_package (IotHubClient REQUIRED)

# NOTE: the call to find_package for azure_c_shared_utility
# must come before umqtt since their config.cmake files expect
# the aziotsharedutil target to already have been defined.
target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)

find_package (umqtt REQUIRED)

target_include_directories (${PROJECT_NAME} PRIVATE ${ADUC_EXPORT_INCLUDES})

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC IotHubClient::iothub_client iothub_client_mqtt_transport umqtt
    PRIVATE aduc::linux_platform_layer
            aduc::extension_manager
            aduc::logging
            aduc::system_utils
            aduc::exception_utils
            aduc::c_utils
            atomic
            Catch2::Catch2
            libaducpal)

include (CTest)
include (Catch)
catch_discover_tests (${PROJECT_NAME})
