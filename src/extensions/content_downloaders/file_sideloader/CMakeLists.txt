cmake_minimum_required(VERSION 3.14)
project(file_sideloader C)

add_library(file_sideloader SHARED
    file_sideloader.c
)

target_include_directories(file_sideloader
    PRIVATE ${CMAKE_SOURCE_DIR}/src/extension_sdk/inc
)

target_link_libraries(file_sideloader
    PRIVATE aduc::extension_sdk
)

target_compile_options(file_sideloader
    PRIVATE
        $<$<C_COMPILER_ID:GNU>:-Wall -Werror>
        $<$<C_COMPILER_ID:Clang>:-Wall -Werror>
)

set_target_properties(file_sideloader PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    OUTPUT_NAME "file_sideloader"
    PREFIX "lib"
)
