cmake_minimum_required(VERSION 3.14)
project(my_step_handler C)

find_package(ADUCEDK REQUIRED)

add_library(my_step_handler SHARED
    step_handler_template.c
)

target_link_libraries(my_step_handler PRIVATE ADUCEDK::edk)

install(TARGETS my_step_handler
    LIBRARY DESTINATION lib/adu/extensions
)
