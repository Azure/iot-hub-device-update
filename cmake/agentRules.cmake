# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

macro (compileAsC99)
    set (CMAKE_C_STANDARD 99)
    set (CMAKE_CXX_STANDARD 11)
endmacro (compileAsC99)

macro (disableRTTI)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        string (
            REGEX
            REPLACE "/GR"
                    ""
                    CMAKE_CXX_FLAGS
                    "${CMAKE_CXX_FLAGS}")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")
    else ()
        string (
            REGEX
            REPLACE "-frtti"
                    ""
                    CMAKE_CXX_FLAGS
                    "${CMAKE_CXX_FLAGS}")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
    endif ()
endmacro (disableRTTI)

function (target_link_aziotsharedutil target scope)
    find_package (azure_c_shared_utility REQUIRED CONFIG)
    target_link_libraries (${target} ${scope} aziotsharedutil)

    # azure_c_shared_utility uses azure_macro_utils, but with VCPKG it doesn't have a dependency?
    # Without this, build errors out with:
    # azure_c_shared_utility/map.h(18,10): fatal  error C1083: Cannot open include file: 'azure_macro_utils/macro_utils.h'
    find_package (azure_macro_utils_c REQUIRED CONFIG)
    target_link_libraries (${target} ${scope} azure_macro_utils_c)
endfunction ()

function (target_link_umock_c target scope)
    find_package (umock_c REQUIRED CONFIG)

    # [START] UMOCK_C VCPKG WORKAROUND
    get_target_property (umock_c_debug_link_libs umock_c IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG)
    list (REMOVE_ITEM umock_c_debug_link_libs azure_macro_utils_c)
    set_property (TARGET umock_c PROPERTY IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG
                                          ${umock_c_debug_link_libs})

    get_target_property (umock_c_release_link_libs umock_c
                         IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE)
    list (REMOVE_ITEM umock_c_release_link_libs azure_macro_utils_c)
    set_property (TARGET umock_c PROPERTY IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG
                                          ${umock_c_release_link_libs})
    # [END] UMOCK_C VCPKG WORKAROUND

    target_link_libraries (${target} ${scope} umock_c)
endfunction ()

function (target_link_iothub_client_mqtt_transport target scope)
    if (WIN32)
        # TODO(JeffMill): Avoid cannot open file 'iothub_client_mqtt_transport.lib'
        find_library (IOTHUB_CLIENT_MQTT_TRANSPORT_LIBRARY iothub_client_mqtt_transport REQUIRED)
        cmake_path (
            GET
            IOTHUB_CLIENT_MQTT_TRANSPORT_LIBRARY
            PARENT_PATH
            IOTHUB_CLIENT_MQTT_TRANSPORT_LIBRARY_DIR)
        message (
            STATUS
                "${target} IOTHUB_CLIENT_MQTT_TRANSPORT_LIBRARY_DIR: ${IOTHUB_CLIENT_MQTT_TRANSPORT_LIBRARY_DIR}"
        )
        target_link_directories (${target} ${scope} ${IOTHUB_CLIENT_MQTT_TRANSPORT_LIBRARY_DIR})
    endif ()
endfunction ()

function (target_link_iothub_client target scope)
    # NOTE: the call to find_package for azure_c_shared_utility
    # must come before umqtt since their config.cmake files expect
    # the aziotsharedutil target to already have been defined.
    find_package (azure_c_shared_utility REQUIRED)
    find_package (IotHubClient REQUIRED)
    target_link_libraries (
        ${target}
        ${scope}
        IotHubClient::iothub_client
        aziotsharedutil
        iothub_client_http_transport
        uhttp)
endfunction (target_link_iothub_client)

function (target_link_digital_twin_client target scope)
    target_link_libraries (${target} ${scope})
    target_link_iothub_client (${target} ${scope})
endfunction (target_link_digital_twin_client)

function (target_link_dosdk target scope)
    if (NOT WIN32)
        find_package (deliveryoptimization_sdk CONFIG REQUIRED)
        target_link_libraries (${target} ${scope} Microsoft::deliveryoptimization)
    else ()
        message (STATUS "[Windows] Not building deliveryoptimization_sdk")
    endif ()
endfunction (target_link_dosdk)
