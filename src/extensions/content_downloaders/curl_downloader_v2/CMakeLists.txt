cmake_minimum_required(VERSION 3.14)
project(curl_downloader_v2 C)

add_library(curl_downloader_v2 SHARED
    curl_downloader_v2.c
)

target_include_directories(curl_downloader_v2
    PRIVATE ${CMAKE_SOURCE_DIR}/src/extension_sdk/inc
)

find_package(CURL REQUIRED)

target_link_libraries(curl_downloader_v2
    PRIVATE aduc::extension_sdk
    PRIVATE CURL::libcurl
)

target_compile_options(curl_downloader_v2
    PRIVATE
        $<$<C_COMPILER_ID:GNU>:-Wall -Werror>
        $<$<C_COMPILER_ID:Clang>:-Wall -Werror>
)

set_target_properties(curl_downloader_v2 PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    OUTPUT_NAME "curl_downloader_v2"
    PREFIX "lib"
)
