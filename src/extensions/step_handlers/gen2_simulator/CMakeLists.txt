cmake_minimum_required(VERSION 3.14)
project(gen2_simulator_handler C)

add_library(gen2_simulator_handler SHARED
    src/simulator_handler.c
)

target_include_directories(gen2_simulator_handler
    PRIVATE inc
    PRIVATE ${CMAKE_SOURCE_DIR}/src/extension_sdk/inc
)

target_link_libraries(gen2_simulator_handler
    PRIVATE aduc::extension_sdk
)

target_compile_options(gen2_simulator_handler
    PRIVATE
        $<$<C_COMPILER_ID:GNU>:-Wall -Werror>
        $<$<C_COMPILER_ID:Clang>:-Wall -Werror>
)

set_target_properties(gen2_simulator_handler PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    OUTPUT_NAME "gen2_simulator_handler"
    PREFIX "lib"
)
