# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

macro (compileAsC99)
    if (CMAKE_VERSION VERSION_LESS "3.1")
        if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
            set (CMAKE_C_FLAGS "--std=c99 ${CMAKE_C_FLAGS}")
            if (CXX_FLAG_CXX11)
                set (CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
            else ()
                set (CMAKE_CXX_FLAGS "--std=c++0x ${CMAKE_CXX_FLAGS}")
            endif ()
        endif ()
    else ()
        set (CMAKE_C_STANDARD 99)
        set (CMAKE_CXX_STANDARD 11)
    endif ()
endmacro (compileAsC99)

macro (disableRTTI)
    # Disable RTTI by replacing rtti compile flag with the no-rtti flag in the cpp compiler flags.
    string (
        REGEX
        REPLACE "-frtti"
                ""
                CMAKE_CXX_FLAGS
                "${CMAKE_CXX_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
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
    find_package (deliveryoptimization_sdk CONFIG REQUIRED)
    target_link_libraries (
        ${target}
        ${scope}
        Microsoft::deliveryoptimization)
endfunction (target_link_dosdk)
