cmake_minimum_required(VERSION 3.14)
project(script_handler_v2 C)

add_library(script_handler_v2 SHARED
    src/script_handler_v2.c
)

target_include_directories(script_handler_v2
    PRIVATE inc
    PRIVATE ${CMAKE_SOURCE_DIR}/src/extension_sdk/inc
)

target_link_libraries(script_handler_v2
    PRIVATE aduc::extension_sdk
)

target_compile_options(script_handler_v2
    PRIVATE
        $<$<C_COMPILER_ID:GNU>:-Wall -Werror>
        $<$<C_COMPILER_ID:Clang>:-Wall -Werror>
)

set_target_properties(script_handler_v2 PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    OUTPUT_NAME "script_handler_v2"
    PREFIX "lib"
)
