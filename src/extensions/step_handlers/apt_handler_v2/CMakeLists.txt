cmake_minimum_required(VERSION 3.14)
project(apt_handler_v2 C)

add_library(apt_handler_v2 SHARED
    src/apt_handler_v2.c
)

target_include_directories(apt_handler_v2
    PRIVATE inc
    PRIVATE ${CMAKE_SOURCE_DIR}/src/extension_sdk/inc
)

target_link_libraries(apt_handler_v2
    PRIVATE aduc::extension_sdk
)

target_compile_options(apt_handler_v2
    PRIVATE
        $<$<C_COMPILER_ID:GNU>:-Wall -Werror>
        $<$<C_COMPILER_ID:Clang>:-Wall -Werror>
)

set_target_properties(apt_handler_v2 PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    OUTPUT_NAME "apt_handler_v2"
    PREFIX "lib"
)
