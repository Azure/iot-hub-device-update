# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

macro (compileAsC99)
    set (CMAKE_C_STANDARD 99)
    set (CMAKE_CXX_STANDARD 17)
    set (CMAKE_CXX_STANDARD_REQUIRED ON)
endmacro (compileAsC99)

macro (disableRTTI)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        string (REGEX REPLACE "/GR" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")
    else ()
        string (REGEX REPLACE "-frtti" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
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

    if (WIN32)
        # [START] UMOCK_C VCPKG WORKAROUND
        get_target_property (umock_c_debug_link_libs umock_c
                             IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG)
        list (REMOVE_ITEM umock_c_debug_link_libs azure_macro_utils_c)
        set_property (TARGET umock_c PROPERTY IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG
                                              ${umock_c_debug_link_libs})

        get_target_property (umock_c_release_link_libs umock_c
                             IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE)
        list (REMOVE_ITEM umock_c_release_link_libs azure_macro_utils_c)
        set_property (TARGET umock_c PROPERTY IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG
                                              ${umock_c_release_link_libs})
        # [END] UMOCK_C VCPKG WORKAROUND
    endif ()

    target_link_libraries (${target} ${scope} umock_c)
endfunction ()

function (target_link_iothub_client target scope)
    find_package (azure_c_shared_utility REQUIRED)
    find_package (IotHubClient REQUIRED)
    target_link_libraries (${target} ${scope} IotHubClient::iothub_client aziotsharedutil
                           iothub_client_http_transport uhttp)

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
        if (CMAKE_GENERATOR_PLATFORM STREQUAL "")
            set (DO_CLIENT_INSTALL_FOLDER "${VCPKG_INSTALLED_DIR}/do-client")
        else ()
            # Different install folder if cross-compiling - see build.ps1
            set (DO_CLIENT_INSTALL_FOLDER
                 "${VCPKG_INSTALLED_DIR}/do-client-${CMAKE_GENERATOR_PLATFORM}")
        endif ()

        target_include_directories (${target} ${scope} ${DO_CLIENT_INSTALL_FOLDER}/sdk-cpp/include)
        target_link_directories (${target} PUBLIC
                                 ${DO_CLIENT_INSTALL_FOLDER}/cmake/sdk-cpp/${CMAKE_BUILD_TYPE})
        target_link_libraries (${target} ${scope} deliveryoptimization)
    endif ()
endfunction (target_link_dosdk)
