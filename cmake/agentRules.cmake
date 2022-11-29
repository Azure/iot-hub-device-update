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
    if (NOT (CMAKE_SYSTEM_NAME STREQUAL "Windows"))
        find_package (deliveryoptimization_sdk CONFIG REQUIRED)
        target_link_libraries (${target} ${scope} Microsoft::deliveryoptimization)
    else ()
        message (
            FATAL_ERROR
                target_link_dosdk
                referenced
                on
                Windows)
    endif ()
endfunction (target_link_dosdk)
