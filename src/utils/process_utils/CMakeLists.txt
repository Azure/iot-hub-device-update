cmake_minimum_required (VERSION 3.5)

project (process_utils)

add_library (${PROJECT_NAME} STATIC src/process_utils.cpp)
add_library (aduc::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

#
# Turn -fPIC on, in order to use this library in another shared library.
#
set_property (TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories (${PROJECT_NAME} PUBLIC inc)

target_link_libraries (
    ${PROJECT_NAME}
    PRIVATE aduc::logging
            aduc::c_utils
            aduc::config_utils
            aduc::string_utils)

# TODO(JeffMill): This is required for Windows to avoid
# C:\wiot-s1\_vcpkg_installed\x64-windows\include\azureiot\azure_c_shared_utility/strings.h(13,10):
# fatal  error C1083: Cannot open include file: 'umock_c/umock_c_prod.h': No such file or directory
if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    find_package (umock_c REQUIRED CONFIG)
    target_link_libraries (${PROJECT_NAME} PRIVATE umock_c)
endif ()

if ((NOT
     ${ADUC_PLATFORM_LAYER}
     STREQUAL
     "simulator"
    )
    AND ADUC_BUILD_UNIT_TESTS)
    add_subdirectory (tests)
    add_subdirectory (test_helper)
endif ()
