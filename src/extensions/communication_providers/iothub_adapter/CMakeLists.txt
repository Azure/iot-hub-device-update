cmake_minimum_required(VERSION 3.14)
project(iothub_adapter C)

add_library(iothub_adapter SHARED
    src/iothub_adapter.c
)

target_include_directories(iothub_adapter
    PRIVATE inc
    PRIVATE ${CMAKE_SOURCE_DIR}/src/extension_sdk/inc
)

target_link_libraries(iothub_adapter
    PRIVATE aduc::extension_sdk
)

target_compile_options(iothub_adapter
    PRIVATE
        $<$<C_COMPILER_ID:GNU>:-Wall -Werror>
        $<$<C_COMPILER_ID:Clang>:-Wall -Werror>
)

set_target_properties(iothub_adapter PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    OUTPUT_NAME "iothub_adapter"
    PREFIX "lib"
)
