cmake_minimum_required(VERSION 3.14)
project(simulator_comm C)

add_library(simulator_comm SHARED
    simulator_comm.c
)

target_include_directories(simulator_comm
    PRIVATE ${CMAKE_SOURCE_DIR}/src/extension_sdk/inc
)

target_link_libraries(simulator_comm
    PRIVATE aduc::extension_sdk
)

target_compile_options(simulator_comm
    PRIVATE
        $<$<C_COMPILER_ID:GNU>:-Wall -Werror -Wno-format-truncation>
        $<$<C_COMPILER_ID:Clang>:-Wall -Werror>
)

set_target_properties(simulator_comm PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    OUTPUT_NAME "simulator_comm"
    PREFIX "lib"
)
