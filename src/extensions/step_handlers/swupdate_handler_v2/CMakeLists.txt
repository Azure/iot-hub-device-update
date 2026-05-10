cmake_minimum_required(VERSION 3.14)
project(swupdate_handler_v2 C)

add_library(swupdate_handler_v2 SHARED
    src/swupdate_handler_v2.c
)

target_include_directories(swupdate_handler_v2
    PRIVATE inc
    PRIVATE ${CMAKE_SOURCE_DIR}/src/extension_sdk/inc
)

target_link_libraries(swupdate_handler_v2
    PRIVATE aduc::extension_sdk
)

target_compile_options(swupdate_handler_v2
    PRIVATE
        $<$<C_COMPILER_ID:GNU>:-Wall -Werror>
        $<$<C_COMPILER_ID:Clang>:-Wall -Werror>
)

set_target_properties(swupdate_handler_v2 PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    OUTPUT_NAME "swupdate_handler_v2"
    PREFIX "lib"
)
