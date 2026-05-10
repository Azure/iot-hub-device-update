cmake_minimum_required(VERSION 3.14)
project(adu_gen2_agent C)

add_executable(adu_gen2_agent
    ${CMAKE_SOURCE_DIR}/src/extension_sdk/src/agent_main.c
)

target_link_libraries(adu_gen2_agent
    PRIVATE aduc::extension_sdk
)

set_target_properties(adu_gen2_agent PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
