cmake_minimum_required(VERSION 3.14)
project(my_content_processor C)

find_package(ADUCEDK REQUIRED)

add_library(my_content_processor SHARED
    content_processor_template.c
)

target_link_libraries(my_content_processor PRIVATE ADUCEDK::edk)

install(TARGETS my_content_processor
    LIBRARY DESTINATION lib/adu/extensions
)
