# Simple example using the Core SDK
cmake_minimum_required(VERSION 3.5)

project(core_sdk_example
        VERSION 1.0.0
        DESCRIPTION "Azure Device Update Core SDK Example"
        LANGUAGES C)

# Find the installed core SDK
find_package(PkgConfig REQUIRED)
pkg_check_modules(ADUC_CORE_SDK REQUIRED aduc-core-sdk)

# Create the example executable
add_executable(core_sdk_example main.c)

# Link with the core SDK
target_link_libraries(core_sdk_example ${ADUC_CORE_SDK_LIBRARIES})
target_include_directories(core_sdk_example PRIVATE ${ADUC_CORE_SDK_INCLUDE_DIRS})
target_compile_options(core_sdk_example PRIVATE ${ADUC_CORE_SDK_CFLAGS_OTHER})
