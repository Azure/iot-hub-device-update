cmake_minimum_required(VERSION 3.14)
project(my_communication_provider C)

find_package(ADUCEDK REQUIRED)

add_library(my_comm_provider SHARED
    communication_provider_template.c
)

target_link_libraries(my_comm_provider PRIVATE ADUCEDK::edk)

install(TARGETS my_comm_provider
    LIBRARY DESTINATION lib/adu/extensions
)
