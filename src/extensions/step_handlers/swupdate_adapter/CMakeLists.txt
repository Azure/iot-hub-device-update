cmake_minimum_required(VERSION 3.14)
project(swupdate_adapter C)

add_library(swupdate_adapter SHARED
    src/swupdate_adapter.c
)

target_include_directories(swupdate_adapter
    PRIVATE inc
    PRIVATE ${CMAKE_SOURCE_DIR}/src/extension_sdk/inc
)

target_link_libraries(swupdate_adapter
    PRIVATE aduc::extension_sdk
)

target_compile_options(swupdate_adapter
    PRIVATE
        $<$<C_COMPILER_ID:GNU>:-Wall -Werror>
        $<$<C_COMPILER_ID:Clang>:-Wall -Werror>
)

set_target_properties(swupdate_adapter PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    OUTPUT_NAME "swupdate_adapter"
    PREFIX "lib"
)
