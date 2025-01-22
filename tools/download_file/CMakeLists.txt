cmake_minimum_required(VERSION 3.22)

set(target_name download_file)

add_executable (${target_name})
target_sources (${target_name} PRIVATE main.cpp)
target_include_directories (${target_name} PUBLIC ../../src/inc)

find_package (deliveryoptimization_sdk CONFIG REQUIRED)
target_link_libraries (${target_name} PRIVATE Microsoft::deliveryoptimization)
