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
    "0"
    STRING
    "The major part of the semantic version")

set_cache_with_env_or_default (
    ADUC_VERSION_MINOR
    "6"
    STRING
    "The minor part of the semantic version")

set_cache_with_env_or_default (
    ADUC_VERSION_PATCH
    "0"
    STRING
    "The patch part of the semantic version")

set_cache_with_env_or_default (
    ADUC_VERSION_PRERELEASE
    "public-preview"
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
    "adu-conf.txt"
    CACHE STRING "Name of the ADU Agent configuration file.")

set (
    ADUC_DATA_FOLDER
    "/var/lib/adu"
    CACHE
        STRING
        "Path to the folder containing data file(s) for the ADU Agent. e.g. installedcriteria file."
)
set (
    ADUC_CONTENT_HANDLERS
    "microsoft/swupdate"
    CACHE STRING "The list of content handlers.")

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
    ADUC_LOGGING_LIBRARY
    "zlog"
    CACHE STRING "The logging library to use. Options: xlog zlog")

set (
    ADUC_LOG_FOLDER
    "/var/log/adu"
    CACHE STRING "Location where ADU Agent will write logs.")

set (
    ADUC_PLATFORM_LAYER
    "simulator"
    CACHE STRING "The platform layer to use. Options: simulator linux")

set (
    ADUC_VERSION_FILE
    "/etc/adu-version"
    CACHE STRING "Path to the file that contains version info for the image or device.")

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
    ADUC_PVINSTALLED_FILE
    "pvinstalled"
    CACHE STRING "Name of the data file containing latest Pantavisor revision info.")

set (
    ADUC_PVPROGRESS_FILE
    "pvprogress"
    CACHE STRING "Name of the data file containing Pantavisior progress for ongoing update.")

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

option (ADUC_WARNINGS_AS_ERRORS "Treat warnings as errors (-Werror)" ON)
option (ADUC_BUILD_UNIT_TESTS "Build unit tests and mock some functionality" OFF)
option (ADUC_BUILD_DOCUMENTATION "Build documentation files" OFF)
option (ADUC_BUILD_PACKAGES "Build the ADU Agent packages" OFF)
option (ADUC_INSTALL_DAEMON "Install the ADU Agent as a daemon" ON)
option (ADUC_PROVISION_WITH_EIS "Provision the connection string with eis" OFF)
option (ADUC_REGISTER_DAEMON "Register the ADU Agent daemon with the system" ON)

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

if (ADUC_BUILD_UNIT_TESTS)
    # Need to be in the root directory to place CTestTestfile.cmake in root
    # of output folder.
    enable_testing ()
endif ()

if (ADUC_INSTALL_DAEMON)
    add_subdirectory (daemon)
endif ()

add_subdirectory (src)

if (ADUC_BUILD_PACKAGES)
    add_subdirectory (packages)
endif ()
