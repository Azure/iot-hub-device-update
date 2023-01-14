cmake_minimum_required (VERSION 3.5)

project (azure_device_update_client)

# Print out CMake build variables.
message (STATUS)
message (STATUS CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME})
message (STATUS CMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION})
message (STATUS CMAKE_HOST_SYSTEM_NAME=${CMAKE_HOST_SYSTEM_NAME})
message (STATUS CMAKE_GENERATOR=${CMAKE_GENERATOR})
message (STATUS CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
message (STATUS CMAKE_C_COMPILER=${CMAKE_C_COMPILER})
message (STATUS CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER})
message (STATUS CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR})
message (STATUS CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR})
message (STATUS CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
message (STATUS CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME})
message (STATUS)

# Tell CMake where to find our custom CMake module files.
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# GNUInstallDirs defines the standard GNU install directories
# so that we can reference them in our install commands.
# e.g. CMAKE_INSTALL_BINDIR
include (GNUInstallDirs)

include (aduc_helpers)

#
# Version information
#

# ADU Agent follows Semantic Versioning 2.0.0
# https://semver.org/spec/v2.0.0.html
set_cache_with_env_or_default (
    ADUC_VERSION_MAJOR
    "1"
    STRING
    "The major part of the semantic version")

set_cache_with_env_or_default (
    ADUC_VERSION_MINOR
    "0"
    STRING
    "The minor part of the semantic version")

set_cache_with_env_or_default (
    ADUC_VERSION_PATCH
    "1"
    STRING
    "The patch part of the semantic version")

set_cache_with_env_or_default (
    ADUC_VERSION_PRERELEASE
    ""
    STRING
    "The pre-release part of the semantic version")

set_cache_with_env_or_default (
    ADUC_VERSION_BUILD
    ""
    STRING
    "The build ID part of the semantic version")

set_cache_with_env_or_default (
    ENABLE_ADU_TELEMETRY_REPORTING
    ON
    BOOL
    "The flag to turn on/off reporting versions to telemetry")

set_cache_with_env_or_default (
    ADUC_BUILDER_IDENTIFIER
    "DU"
    STRING
    "The ADUC builder name: ADUC-sourced builder will be called DU")

set (ADUC_VERSION "${ADUC_VERSION_MAJOR}.${ADUC_VERSION_MINOR}.${ADUC_VERSION_PATCH}")

if (ADUC_VERSION_PRERELEASE)
    string (APPEND ADUC_VERSION "-${ADUC_VERSION_PRERELEASE}")
endif ()

if (ADUC_VERSION_BUILD)
    string (APPEND ADUC_VERSION "+${ADUC_VERSION_BUILD}")
endif ()

#
# Custom CMake Options
#

set (
    ADUC_CONF_FOLDER
    "/etc/adu"
    CACHE STRING "Path to the configuration folder for the ADU Agent.")

set (
    ADUC_CONF_FILE
    "du-config.json"
    CACHE STRING "Name of the ADU Agent configuration file.")

set (
    ADUC_DATA_FOLDER
    "/var/lib/adu"
    CACHE
        STRING
        "Path to the folder containing data file(s) for the ADU Agent. e.g. installedcriteria file."
)

set (
    ADUC_TEST_DATA_PATH_SEGMENT
    "testdata"
    CACHE STRING
          "The path segment in source tree where testdata for various test folders will be found.")

set (
    ADUC_TEST_DATA_FOLDER
    "/tmp/adu/${ADUC_TEST_DATA_PATH_SEGMENT}"
    CACHE
        STRING
        "Path to the folder containing test data file(s) copied from tests/testdata/<testname> folders in the source tree."
)

set (
    ADUC_EXTENSIONS_FOLDER
    "${ADUC_DATA_FOLDER}/extensions"
    CACHE STRING "Path to the folder containing file(s) related to DU Agent extension(s).")

set (
    ADUC_EXTENSIONS_INSTALL_FOLDER
    "${ADUC_EXTENSIONS_FOLDER}/sources"
    CACHE STRING "Path to the folder containing extensions .so file(s).")

set (
    ADUC_UPDATE_CONTENT_HANDLER_REG_FILENAME
    "content_handler.json"
    CACHE STRING "Content handler registration file name.")

set (
    ADUC_DOWNLOAD_HANDLER_REG_FILENAME
    "download_handler.json"
    CACHE STRING "Download handler registration file name.")

set (
    ADUC_EXTENSION_REG_FILENAME
    "extension.json"
    CACHE STRING "Extension registration file name.")

set (
    ADUC_EXTENSIONS_SUBDIR_COMPONENT_ENUMERATOR
    "component_enumerator"
    CACHE STRING "Sub folder for component enumerator extension.")

set (
    ADUC_EXTENSIONS_SUBDIR_CONTENT_DOWNLOADER
    "content_downloader"
    CACHE STRING "Sub folder for content downloader extension.")

set (
    ADUC_EXTENSIONS_SUBDIR_UPDATE_CONTENT_HANDLERS
    "update_content_handlers"
    CACHE STRING "Sub folder for update content handler extensions.")

set (
    ADUC_COMPONENT_ENUMERATOR_EXTENSION_DIR
    "${ADUC_EXTENSIONS_FOLDER}/${ADUC_EXTENSIONS_SUBDIR_COMPONENT_ENUMERATOR}"
    CACHE STRING "Path for component enumerator extension.")

set (
    ADUC_CONTENT_DOWNLOADER_EXTENSION_DIR
    "${ADUC_EXTENSIONS_FOLDER}/${ADUC_EXTENSIONS_SUBDIR_CONTENT_DOWNLOADER}"
    CACHE STRING "Path for content downloader extension.")

set (
    ADUC_UPDATE_CONTENT_HANDLER_EXTENSION_DIR
    "${ADUC_EXTENSIONS_FOLDER}/${ADUC_EXTENSIONS_SUBDIR_UPDATE_CONTENT_HANDLERS}"
    CACHE STRING "Sub folder for update content handler extensions.")

set (
    ADUC_EXTENSIONS_SUBDIR_DOWNLOAD_HANDLERS
    "download_handlers"
    CACHE STRING "Sub folder for download handler extensions.")

set (
    ADUC_DOWNLOAD_HANDLER_EXTENSION_DIR
    "${ADUC_EXTENSIONS_FOLDER}/${ADUC_EXTENSIONS_SUBDIR_DOWNLOAD_HANDLERS}"
    CACHE STRING "Path for the download handler extensions")

set (
    ADUC_DOWNLOADS_FOLDER
    "${ADUC_DATA_FOLDER}/downloads"
    CACHE STRING "Path to the folder containing downloaded update artifacts.")

set (
    ADUC_CONTENT_HANDLERS
    "microsoft/swupdate"
    CACHE STRING "The list of content handlers.")

set (
    ADUC_COMMANDS_FIFO_NAME
    "${ADUC_DATA_FOLDER}/du-commands.fifo"
    CACHE STRING "The named-pipe for commands IPC.")

#
# Starting from version 0.8.1, Device Update Agent must support only one version of the update manifest
# based on the base dtmi model that the Agent announces when connecting to the IoT Hub.
#
# For this version, "dtmi:azure:iot:deviceUpdateModel;2" is using the manifest version verison 5.
#
set (SUPPORTED_UPDATE_MANIFEST_VERSION_MIN 4)
set (SUPPORTED_UPDATE_MANIFEST_VERSION_MAX 5)

#
# DeviceInfo configuration
# These values must be modified to describe your device.
#

set (
    ADUC_DEVICEINFO_MANUFACTURER
    "Contoso"
    CACHE STRING "Manufacturer of the device.")

set (
    ADUC_DEVICEINFO_MODEL
    "Virtual Machine"
    CACHE STRING "Name of the device.")

#
# AzureDeviceUpdateInterface DeviceProperties configuration
#

set (
    ADUC_DEVICEPROPERTIES_MANUFACTURER
    "Contoso"
    CACHE STRING "Manufacturer of the device")

set (
    ADUC_DEVICEPROPERTIES_MODEL
    "Video"
    CACHE STRING "Model of the device")

set (
    ADUC_PLATFORM_LAYER
    "linux"
    CACHE STRING "The platform layer to use. Options: simulator linux")

# By default ADU Agent daemon runs as root.
# root user home directory should be /root.
set (
    ADUC_HOME_FOLDER
    "/root"
    CACHE STRING "Path to the ADU Agent home folder.")

set (
    ADUC_INSTALLEDCRITERIA_FILE
    "installedcriteria"
    CACHE STRING "Name of the data file containing InstalledCriteria records.")

set (
    ADUC_LOGGING_LIBRARY
    "zlog"
    CACHE STRING "The logging library to use. Options: xlog zlog")

set (
    ADUC_LOG_FOLDER
    "/var/log/adu"
    CACHE STRING "Location where ADU Agent will write logs.")

set (
    ADUC_VERSION_FILE
    "/etc/adu-version"
    CACHE STRING "Path to the file that contains version info for the image or device.")

set (
    ADUC_AGENT_FOLDER
    "/usr/bin"
    CACHE STRING "Location where ADU Agent binary is located.")

set (
    ADUC_AGENT_FILENAME
    "AducIotAgent"
    CACHE STRING "The filename of the ADU Agent binary.")

get_filename_component (
    ADUC_AGENT_FILEPATH
    "${ADUC_AGENT_FOLDER}/${ADUC_AGENT_FILENAME}"
    ABSOLUTE
    "/")

set (
    DIAGNOSTICS_CONFIG_FILE
    "du-diagnostics-config.json"
    CACHE STRING
          "Path to the file that contains the configuration information for the diagnostics system")

set (
    DIAGNOSTICS_COMPLETED_OPERATION_FILE
    "diagnosticsoperationids"
    CACHE STRING "File that contains the last completed diagnostic operation's id")
set (
    DIAGNOSTICS_COMPLETED_OPERATION_FILE_PATH
    "${ADUC_DATA_FOLDER}/${DIAGNOSTICS_COMPLETED_OPERATION_FILE}"
    CACHE STRING "Path to the file that contains the completed diagnostic operation ids")

set (
    ADUSHELL_FOLDER
    "/usr/lib/adu"
    CACHE STRING "The directory of the adu-shell binary.")

set (
    ADUSHELL_FILENAME
    "adu-shell"
    CACHE STRING "Filename of adu-shell.")

get_filename_component (
    ADUSHELL_FILE_PATH
    "${ADUSHELL_FOLDER}/${ADUSHELL_FILENAME}"
    ABSOLUTE
    "/")

set (
    ADUSHELL_FILE_PATH
    ${ADUSHELL_FILE_PATH}
    CACHE STRING "The absolute path to the adu-shell file path.")

set (
    ADUSHELL_EFFECTIVE_GROUP_NAME
    "adu"
    CACHE STRING "The effective group name for adu-shell process.")

set (
    ADUC_FILE_USER
    "adu"
    CACHE STRING "The user for adu file ownership.")

set (
    ADUC_FILE_GROUP
    "adu"
    CACHE STRING "The group for adu file ownership.")

set (
    DO_FILE_USER
    "do"
    CACHE STRING "The user for do file ownership.")

set (
    DO_FILE_GROUP
    "do"
    CACHE STRING "The group for do file ownership.")

set (
    SYSLOG_FILE_GROUP
    "syslog"
    CACHE STRING "The syslog group.")

set (
    ADUC_IOT_HUB_PROTOCOL
    "IotHub_Protocol_from_Config"
    CACHE
        STRING
        "The protocol for Azure IotHub SDK communication. Options are MQTT, MQTT_over_WebSockets, and IotHub_Protocol_from_Config"
)

# Device Update Repo Locations

set (
    ADUC_SCRIPTS_FOLDER
    "/scripts/"
    CACHE STRING "The folder that holds all of the scripts")

set (
    ADUC_SCRIPTS_FOLDER_PATH
    "${PROJECT_SOURCE_DIR}/${ADUC_SCRIPTS_FOLDER}"
    CACHE STRING "Path to the scripts folder within the Device Update project")

# Error Code Generator Script Name
set (
    ADUC_ERROR_CODE_GENERATOR_SCRIPT_FILENAME
    "generate_error_code_defs.sh"
    CACHE STRING "The script file that generates the error code definitions")

# Error Code Definition Folder and Filename
set (
    ADUC_ERROR_CODE_DEFS_FOLDER
    "/src/inc/aduc/"
    CACHE STRING "The folder within Device Update to generate the result.h file to")

set (
    ADUC_GENERATED_ERROR_CODE_FOLDER_PATH
    "${PROJECT_SOURCE_DIR}/${ADUC_ERROR_CODE_DEFS_FOLDER}"
    CACHE STRING "Path to the folder to generate the error code file")

set (
    ADUC_ERROR_CODE_DEFS_FILENAME
    "result.h"
    CACHE STRING "File name for the error code definitions")

# Error Code Json Folder, Filename, and Path
set (
    ADUC_ERROR_CODE_GENERATOR_JSON_FOLDER
    "error_code_generator_defs"
    CACHE STRING "Name of the folder that contains the error codes JSON file")

set (
    ADUC_ERROR_CODE_JSON_FILENAME
    "result_codes.json"
    CACHE STRING "Name of the json file that defines the error codes for Device Update")

set (
    ADUC_ERROR_CODE_JSON_FOLDER_PATH
    "${ADUC_SCRIPTS_FOLDER_PATH}/${ADUC_ERROR_CODE_GENERATOR_JSON_FOLDER}"
    CACHE STRING "Path to the folder where the JSON error code definition file lives")

# Paths for the Folders

get_filename_component (
    ADUC_PATH_TO_ERROR_CODE_GENERATOR_SCRIPT
    "${ADUC_SCRIPTS_FOLDER_PATH}/${ADUC_ERROR_CODE_GENERATOR_SCRIPT_FILENAME}"
    ABSOLUTE
    "/")

get_filename_component (
    ADUC_PATH_TO_ERROR_CODE_JSON_FILE
    "${ADUC_ERROR_CODE_JSON_FOLDER_PATH}/${ADUC_ERROR_CODE_JSON_FILENAME}"
    ABSOLUTE
    "/")

get_filename_component (
    ADUC_GENERATED_ERROR_CODE_FILE_PATH
    "${ADUC_GENERATED_ERROR_CODE_FOLDER_PATH}/${ADUC_ERROR_CODE_DEFS_FILENAME}"
    ABSOLUTE
    "/")

#######
# BEGIN Delta Downloader Handler Source Update Cache Configurations

# Source Update Cache Commit Strategy
#
# Value values are:
#   "TWO_PHASE_COMMIT" - Moves the incoming files then deletes previous.
#   "OVERWRITE" - Saves space by deleting existing source updates first.
set (
    ADUC_DELTA_DOWNLOAD_HANDLER_SOURCE_UPDATE_CACHE_COMMIT_STRATEGY
    "TWO_PHASE_COMMIT"
    CACHE STRING "The commit strategy for the source update cache.")

# Source Update Cache Directory
#
# Directory where an update from download work folder is cached after successful workflow processing
set (
    ADUC_DELTA_DOWNLOAD_HANDLER_SOURCE_UPDATE_CACHE_DIR
    "${ADUC_DATA_FOLDER}/sdc"
    CACHE STRING
          "The base directory to cache source update payloads that delta updates are based upon.")

# END Delta Downloader Handler Source Update Cache Configurations
#######

option (ADUC_WARNINGS_AS_ERRORS "Treat warnings as errors (-Werror)" ON)
option (ADUC_BUILD_UNIT_TESTS "Build unit tests and mock some functionality" OFF)
option (ADUC_BUILD_DOCUMENTATION "Build documentation files" OFF)
option (ADUC_BUILD_PACKAGES "Build the ADU Agent packages" OFF)
option (ADUC_INSTALL_DAEMON "Install the ADU Agent as a daemon" ON)
option (ADUC_REGISTER_DAEMON "Register the ADU Agent daemon with the system" ON)
option (ADUC_TRACE_TARGET_DEPS "Trace target dependencies" OFF)

### End CMake Options

# Construct the absolute path to the adu configuration file.
get_filename_component (
    ADUC_CONF_FILE_PATH
    "${ADUC_CONF_FOLDER}/${ADUC_CONF_FILE}"
    ABSOLUTE
    "/")
set (
    ADUC_CONF_FILE_PATH
    "${ADUC_CONF_FILE_PATH}"
    CACHE STRING "Path to the ADU configuration file.")

# Construct the absolute path to the adu diagnostics configuration file
get_filename_component (
    DIAGNOSTICS_CONFIG_FILE_PATH
    "${ADUC_CONF_FOLDER}/${DIAGNOSTICS_CONFIG_FILE}"
    ABSOLUTE
    "/")

set (
    DIAGNOSTICS_CONFIG_FILE_PATH
    "${DIAGNOSTICS_CONFIG_FILE_PATH}"
    CACHE STRING "Path to the diagnostics configuration file.")

# Generate the error code definition file
execute_process (
    COMMAND
        ${ADUC_PATH_TO_ERROR_CODE_GENERATOR_SCRIPT} --json-file-path
        "${ADUC_PATH_TO_ERROR_CODE_JSON_FILE}" --result-file-path
        "${ADUC_GENERATED_ERROR_CODE_FILE_PATH}" -s
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE GENERATE_ERROR_CODE_DEFS_RESULT)

if (NOT
    GENERATE_ERROR_CODE_DEFS_RESULT
    EQUAL
    "0")
    message (
        FATAL_ERROR
            "Failed to generate error code definition from json file ${ADUC_PATH_TO_ERROR_CODE_JSON_FILE} to ${ADUC_GENERATED_ERROR_CODE_FILE_PATH} with error code ${GENERATE_ERROR_CODE_DEFS_RESULT}"
    )
endif ()

if (ADUC_BUILD_UNIT_TESTS)
    # Need to be in the root directory to place CTestTestfile.cmake in root
    # of output folder.
    enable_testing ()

    copy_test_data ("${CMAKE_SOURCE_DIR}" "${ADUC_TEST_DATA_PATH_SEGMENT}"
                    "${ADUC_TEST_DATA_FOLDER}")
endif ()

if (ADUC_INSTALL_DAEMON)
    add_subdirectory (daemon)
endif ()

add_subdirectory (src)

if (ADUC_BUILD_PACKAGES)
    add_subdirectory (packages)
endif ()
