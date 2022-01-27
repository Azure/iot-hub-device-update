# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Find cmake module for the iothub_client library and headers
# Exports IotHubClient::iothub_client target

cmake_minimum_required (VERSION 3.5)

include (FindPackageHandleStandardArgs)

find_path (
    IotHubClient_INCLUDE_DIR
    NAMES iothub_client.h
    PATH_SUFFIXES azureiot azureiot/inc)

find_library (IotHubClient_LIBRARY iothub_client)

find_package_handle_standard_args (
    IotHubClient
    DEFAULT_MSG
    IotHubClient_INCLUDE_DIR
    IotHubClient_LIBRARY)

if (IotHubClient_FOUND)
    set (IotHubClient_INCLUDE_DIRS ${IotHubClient_INCLUDE_DIR})
    set (IotHubClient_LIBRARIES ${IotHubClient_LIBRARY})

    if (NOT TARGET IotHubClient::iothub_client)
        add_library (IotHubClient::iothub_client INTERFACE IMPORTED)
        set_target_properties (
            IotHubClient::iothub_client
            PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${IotHubClient_INCLUDE_DIRS}"

                       INTERFACE_LINK_LIBRARIES "${IotHubClient_LIBRARIES}")
    endif ()
endif ()
