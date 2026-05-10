cmake_minimum_required(VERSION 3.14)
project(adu_direct_comm C)

add_library(adu_direct_comm SHARED
    src/adu_direct_comm.c
)

set_target_properties(adu_direct_comm PROPERTIES
    OUTPUT_NAME "adu_direct_comm"
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS ON
    PREFIX ""
)

target_include_directories(adu_direct_comm
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_compile_options(adu_direct_comm PRIVATE
    -Wall
    -Werror
)

find_package(CURL REQUIRED)

target_link_libraries(adu_direct_comm
    PRIVATE
        aduc::extension_sdk
        CURL::libcurl
)
