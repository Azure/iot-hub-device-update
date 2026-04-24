#
# FindAzureIotHubDeviceUpdateDelta.cmake
#
# Finds the Azure IoT Hub Device Update Delta library.
#
# This will define the following variables:
#
#   AzureIotHubDeviceUpdateDelta_FOUND    - System has the delta library
#   AzureIotHubDeviceUpdateDelta_INCLUDE_DIRS - The delta library include directories
#   AzureIotHubDeviceUpdateDelta_LIBRARIES - The delta library libraries
#
# And the following imported targets:
#
#   AzureIotHubDeviceUpdateDelta::delta

include (FindPackageHandleStandardArgs)

# First, try to find in system locations (/usr, /usr/local), then fall back to build directories
set (DELTA_ROOT_HINTS
     /usr
     /usr/local
     $ENV{HOME}/.adu-tmp/iot-hub-device-update-delta
     ${CMAKE_SOURCE_DIR}/../.adu-tmp/iot-hub-device-update-delta
     ${CMAKE_PREFIX_PATH})

# Find the include directory
find_path (
    AzureIotHubDeviceUpdateDelta_INCLUDE_DIR
    NAMES adudiffapi.h
    HINTS ${DELTA_ROOT_HINTS}
    PATH_SUFFIXES include inc src/native/diffs/api
    DOC "Azure IoT Hub Device Update Delta library include directory")

# Find the library
find_library (
    AzureIotHubDeviceUpdateDelta_LIBRARY
    NAMES libadudiffapi.so libadudiffapi.a adudiffapi
    HINTS ${DELTA_ROOT_HINTS}
    PATH_SUFFIXES lib lib64 bin src/out/native/x64-linux/Debug/bin src/out/native/x64-linux/Release/bin
    DOC "Azure IoT Hub Device Update Delta library")

# Handle standard arguments
find_package_handle_standard_args (
    AzureIotHubDeviceUpdateDelta
    REQUIRED_VARS AzureIotHubDeviceUpdateDelta_LIBRARY AzureIotHubDeviceUpdateDelta_INCLUDE_DIR
    VERSION_VAR AzureIotHubDeviceUpdateDelta_VERSION)

if (AzureIotHubDeviceUpdateDelta_FOUND)
    set (AzureIotHubDeviceUpdateDelta_INCLUDE_DIRS ${AzureIotHubDeviceUpdateDelta_INCLUDE_DIR})
    set (AzureIotHubDeviceUpdateDelta_LIBRARIES ${AzureIotHubDeviceUpdateDelta_LIBRARY})

    # Create imported target
    if (NOT TARGET AzureIotHubDeviceUpdateDelta::delta)
        add_library (AzureIotHubDeviceUpdateDelta::delta UNKNOWN IMPORTED)
        set_target_properties (
            AzureIotHubDeviceUpdateDelta::delta
            PROPERTIES IMPORTED_LOCATION "${AzureIotHubDeviceUpdateDelta_LIBRARY}"
                       INTERFACE_INCLUDE_DIRECTORIES
                       "${AzureIotHubDeviceUpdateDelta_INCLUDE_DIR}")
    endif ()

    mark_as_advanced (AzureIotHubDeviceUpdateDelta_INCLUDE_DIR
                      AzureIotHubDeviceUpdateDelta_LIBRARY)
endif ()
