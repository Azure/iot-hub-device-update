cmake_minimum_required(VERSION 3.14)
project(static_file_enumerator C)

add_library(static_file_enumerator SHARED
    src/static_enumerator.c
)

target_include_directories(static_file_enumerator
    PRIVATE inc
    PRIVATE ${CMAKE_SOURCE_DIR}/src/extension_sdk/inc
)

target_link_libraries(static_file_enumerator
    PRIVATE aduc::extension_sdk
    PRIVATE tomlc99
)

target_compile_options(static_file_enumerator
    PRIVATE
        $<$<C_COMPILER_ID:GNU>:-Wall -Werror>
        $<$<C_COMPILER_ID:Clang>:-Wall -Werror>
)

set_target_properties(static_file_enumerator PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    OUTPUT_NAME "static_file_enumerator"
    PREFIX "lib"
)
