cmake_minimum_required(VERSION 3.14)
project(delta_processor C)

add_library(delta_processor SHARED
    delta_processor.c
)

target_include_directories(delta_processor
    PRIVATE ${CMAKE_SOURCE_DIR}/src/extension_sdk/inc
)

target_link_libraries(delta_processor
    PRIVATE aduc::extension_sdk
)

target_compile_options(delta_processor
    PRIVATE
        $<$<C_COMPILER_ID:GNU>:-Wall -Werror>
        $<$<C_COMPILER_ID:Clang>:-Wall -Werror>
)

set_target_properties(delta_processor PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    OUTPUT_NAME "delta_processor"
    PREFIX "lib"
)
