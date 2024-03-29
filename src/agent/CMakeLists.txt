cmake_minimum_required (VERSION 3.5)

project (azure_iot_aduc_agent)

set (target_name AducIotAgent)

add_subdirectory (adu_core_export_helpers)
add_subdirectory (adu_core_interface)
add_subdirectory (device_info_interface)
add_subdirectory (pnp_helper)
add_subdirectory (shutdown_service)

if (NOT WIN32)
    add_subdirectory (command_helper)
endif ()

include (agentRules)

compileasc99 ()
disablertti ()

set (agent_c_files ./src/main.c ./src/health_management.c)

add_executable (${target_name} ${agent_c_files})

set_target_properties (${target_name} PROPERTIES COMPILE_DEFINITIONS _DEFAULT_SOURCE)

# Pick up Git revision so we can report it in version information.

include (FindGit)
if (GIT_FOUND)
    execute_process (
        COMMAND ${GIT_EXECUTABLE} rev-parse --show-toplevel
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_ROOT
        OUTPUT_STRIP_TRAILING_WHITESPACE)
endif ()
if (GIT_ROOT)
    execute_process (
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE ADUC_GIT_HEAD_REVISION
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process (
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE ADUC_GIT_HEAD_NAME
        OUTPUT_STRIP_TRAILING_WHITESPACE)

    target_compile_definitions (
        ${target_name} PRIVATE ADUC_GIT_INFO="${ADUC_GIT_HEAD_NAME}:${ADUC_GIT_HEAD_REVISION}")
else ()
    message (WARNING "Git version info not found, DO NOT release from this build tree!")
endif ()

target_compile_definitions (
    ${target_name}
    PRIVATE ADUC_VERSION="${ADUC_VERSION}"
            ADUC_PLATFORM_LAYER="${ADUC_PLATFORM_LAYER}"
            ADUC_STEP_HANDLERS="${ADUC_STEP_HANDLERS}"
            SUPPORTED_UPDATE_MANIFEST_VERSION_MIN=${SUPPORTED_UPDATE_MANIFEST_VERSION_MIN}
            SUPPORTED_UPDATE_MANIFEST_VERSION_MAX=${SUPPORTED_UPDATE_MANIFEST_VERSION_MAX})

# NOTE: the call to find_package for azure_c_shared_utility
# must come before umqtt since their config.cmake files expect the aziotsharedutil target to already have been defined.
target_link_aziotsharedutil (${target_name} PRIVATE)

find_package (IotHubClient REQUIRED)
find_package (umqtt REQUIRED)
find_package (Parson REQUIRED)

# Windows needs all EXEs to have links to all potential libraries so this is included here
if (WIN32)
    target_link_dosdk (${target_name} PRIVATE)
endif ()

target_link_libraries (
    ${target_name}
    PRIVATE IotHubClient::iothub_client
            umqtt
            aduc::adu_core_export_helpers
            aduc::adu_core_interface
            aduc::agent_orchestration
            aduc::agent_workflow
            aduc::c_utils
            aduc::communication_abstraction
            aduc::config_utils
            aduc::d2c_messaging
            aduc::device_info_interface
            aduc::eis_utils
            aduc::extension_manager
            aduc::extension_utils
            aduc::iothub_communication_manager
            aduc::logging
            aduc::permission_utils
            aduc::pnp_helper
            aduc::shutdown_service
            aduc::system_utils
            aduc::url_utils
            diagnostics_component::diagnostics_interface
            diagnostics_component::diagnostics_devicename)

if (NOT WIN32)
    target_link_libraries (${target_name} PRIVATE aduc::command_helper)
endif ()

target_link_libraries (${target_name} PRIVATE libaducpal)

message (STATUS "ADUC_IOT_HUB_PROTOCOL: ${ADUC_IOT_HUB_PROTOCOL}")

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

get_filename_component (ADUC_INSTALLEDCRITERIA_FILE_PATH
                        "${ADUC_DATA_FOLDER}/${ADUC_INSTALLEDCRITERIA_FILE}" ABSOLUTE "/")

target_compile_definitions (
    ${target_name}
    PRIVATE ADUC_AGENT_FILEPATH="${ADUC_AGENT_FILEPATH}"
            ADUC_COMMANDS_FIFO_NAME="${ADUC_COMMANDS_FIFO_NAME}"
            ADUC_CONF_FILE_PATH="${ADUC_CONF_FILE_PATH}"
            ADUC_CONF_FOLDER="${ADUC_CONF_FOLDER}"
            ADUC_DATA_FOLDER="${ADUC_DATA_FOLDER}"
            ADUC_COMMANDS_FIFO_NAME="${ADUC_COMMANDS_FIFO_NAME}"
            ADUC_FILE_GROUP="${ADUC_FILE_GROUP}"
            ADUC_FILE_USER="${ADUC_FILE_USER}"
            ADUC_INSTALLEDCRITERIA_FILE_PATH="${ADUC_INSTALLEDCRITERIA_FILE_PATH}"
            ADUC_PLATFORM_LAYER="${ADUC_PLATFORM_LAYER}"
            DO_FILE_GROUP="${DO_FILE_GROUP}"
            DO_FILE_USER="${DO_FILE_USER}"
            SYSLOG_FILE_GROUP="${SYSLOG_FILE_GROUP}")

if (WIN32)
    find_package (unofficial-getopt-win32 REQUIRED)
    target_link_libraries (${target_name} PRIVATE unofficial::getopt-win32::getopt)
endif ()

target_link_libraries (${target_name} PRIVATE aduc::platform_layer)

install (TARGETS ${target_name} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
