# Script Handler v3 - SDK Example
# Demonstrates Azure Device Update Step Handler SDK usage

cmake_minimum_required(VERSION 3.5)

project(script_handler_v3
        VERSION 1.0.0
        DESCRIPTION "Azure Device Update Script Handler v3 - SDK Example"
        LANGUAGES C)

# Find the required SDKs
find_package(PkgConfig REQUIRED)
pkg_check_modules(ADUC_CORE_SDK REQUIRED aduc-core-sdk)
pkg_check_modules(CURL REQUIRED libcurl)

# Create the script handler v3 extension as a shared library
add_library(script_handler_v3 SHARED
    script_handler_v3.c
    script_utils.c
)

# Include directories
target_include_directories(script_handler_v3 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ADUC_CORE_SDK_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ../../inc  # Step Handler SDK headers
)

# Link with required libraries
target_link_libraries(script_handler_v3 
    ${ADUC_CORE_SDK_LIBRARIES}
    ${CURL_LIBRARIES}
)

# Compiler flags
target_compile_options(script_handler_v3 PRIVATE 
    ${ADUC_CORE_SDK_CFLAGS_OTHER}
    ${CURL_CFLAGS_OTHER}
)

# Set library properties for extension loading
set_target_properties(script_handler_v3 PROPERTIES
    PREFIX ""  # Remove 'lib' prefix
    SUFFIX ".so"
    VERSION ${PROJECT_VERSION}
)

# Install the extension
install(TARGETS script_handler_v3
        LIBRARY DESTINATION lib/adu/extensions
)

# Create a test executable
add_executable(test_script_handler_v3
    test_script_handler_v3.c
    script_handler_v3.c
    script_utils.c
)

target_include_directories(test_script_handler_v3 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ADUC_CORE_SDK_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

target_link_libraries(test_script_handler_v3 
    ${ADUC_CORE_SDK_LIBRARIES}
    ${CURL_LIBRARIES}
)

target_compile_options(test_script_handler_v3 PRIVATE 
    ${ADUC_CORE_SDK_CFLAGS_OTHER}
    ${CURL_CFLAGS_OTHER}
)