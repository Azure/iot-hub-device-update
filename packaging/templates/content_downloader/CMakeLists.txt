cmake_minimum_required(VERSION 3.14)
project(my_content_downloader C)

find_package(ADUCEDK REQUIRED)

add_library(my_downloader SHARED
    content_downloader_template.c
)

target_link_libraries(my_downloader PRIVATE ADUCEDK::edk)

install(TARGETS my_downloader
    LIBRARY DESTINATION lib/adu/extensions
)
