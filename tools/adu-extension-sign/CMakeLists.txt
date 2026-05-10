cmake_minimum_required(VERSION 3.14)
project(adu-extension-sign C)

find_package(Parson REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(adu-extension-sign
    src/main.c
)

target_include_directories(adu-extension-sign
    PRIVATE ${CMAKE_SOURCE_DIR}/src/extension_sdk/inc
)

target_link_libraries(adu-extension-sign
    PRIVATE aduc_extension_sdk
            Parson::parson
            OpenSSL::Crypto
)
