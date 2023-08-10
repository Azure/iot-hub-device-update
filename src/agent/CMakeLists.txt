cmake_minimum_required (VERSION 3.5)

project (azure_iot_aduc_agent)

set (target_name AducIotAgent)

add_subdirectory (adu_core_export_helpers)
add_subdirectory (command_helper)

include (agentRules)

include (find_curl_and_import_libcurl)

compileasc99 ()
disablertti ()

# Used to find and include the CURL::libcurl imported libary
find_curl_and_import_libcurl ()

set (agent_c_files ./src/main.c ./src/health_management.c)

add_executable (${target_name} ${agent_c_files})

set_target_properties (${target_name} PROPERTIES COMPILE_DEFINITIONS _DEFAULT_SOURCE)

target_include_directories (
    ${target_name}
    PRIVATE inc)

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


find_package (Parson REQUIRED)

target_link_digital_twin_client (${target_name} PRIVATE)

target_link_libraries (
    ${target_name}
    PRIVATE
            aduc::adu_core_export_helpers
            aduc::agent_orchestration
            aduc::agent_workflow
            aduc::c_utils
            aduc::command_helper
            aduc::config_utils
            aduc::eis_utils
            aduc::extension_manager
            aduc::extension_utils
            aduc::https_proxy_utils
            aduc::logging
            aduc::permission_utils
            aduc::system_utils)

get_filename_component (
    ADUC_INSTALLEDCRITERIA_FILE_PATH
    "${ADUC_DATA_FOLDER}/${ADUC_INSTALLEDCRITERIA_FILE}"
    ABSOLUTE
    "/")

target_compile_definitions (
    ${target_name}
    PRIVATE ADUC_AGENT_FILEPATH="${ADUC_AGENT_FILEPATH}"
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
            SYSLOG_FILE_GROUP="${SYSLOG_FILE_GROUP}"
            ADU_GEN2="true")

target_link_libraries (${target_name} PRIVATE aduc::platform_layer)

install (TARGETS ${target_name} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
