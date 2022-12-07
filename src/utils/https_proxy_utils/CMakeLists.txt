cmake_minimum_required (VERSION 3.5)

set (target_name https_proxy_utils)

add_library (${target_name} STATIC src/https_proxy_utils.c)
add_library (aduc::${target_name} ALIAS ${target_name})

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${target_name} PUBLIC inc)

target_link_aziotsharedutil (${target_name} PUBLIC)

# TODO(JeffMill): This is  required for Windows to avoid
# C:\wiot-s1\_vcpkg_installed\x64-windows\include\azureiot\azure_c_shared_utility/strings.h(13,10):
# fatal  error C1083: Cannot open include file: 'umock_c/umock_c_prod.h': No such file or directory
if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    find_package (umock_c REQUIRED CONFIG)
    target_link_libraries (${target_name} PRIVATE umock_c)
endif ()

target_link_libraries (
    ${target_name}
    PUBLIC aduc::c_utils
    PRIVATE aduc::logging)

if (ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
endif ()
