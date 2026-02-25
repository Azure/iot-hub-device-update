cmake_minimum_required (VERSION 3.5)

project (iothub_communication_manager_unit_tests)

include (agentRules)

compileasc99 ()
disablertti ()

find_package (Catch2 REQUIRED)
find_package (IotHubClient REQUIRED)
find_package (umqtt REQUIRED)

add_executable (${PROJECT_NAME} iothub_communication_manager_ut.cpp)

target_include_directories (
    ${PROJECT_NAME}
    PRIVATE ${ADUC_EXPORT_INCLUDES}
            ${ADUC_TYPES_INCLUDES}
            ${CMAKE_CURRENT_SOURCE_DIR}/../inc)

# NOTE: the call to find_package for azure_c_shared_utility
# must come before umqtt since their config.cmake files expect
# the aziotsharedutil target to already have been defined.
target_link_aziotsharedutil (${PROJECT_NAME} PRIVATE)

target_link_libraries (
    ${PROJECT_NAME}
    PUBLIC IotHubClient::iothub_client iothub_client_mqtt_transport iothub_client_mqtt_ws_transport umqtt
    PRIVATE aduc::iothub_communication_manager
            aduc::communication_abstraction
            aduc::adu_types
            aduc::c_utils
            aduc::logging
            atomic
            Catch2::Catch2WithMain
            libaducpal)

target_compile_definitions (${PROJECT_NAME} PRIVATE ADUC_CONF_FILE_PATH="/etc/adu/du-config.json")

include (CTest)
include (Catch)
if (NOT CMAKE_CROSSCOMPILING)
    catch_discover_tests (${PROJECT_NAME})
else ()
    add_test (NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
endif ()
