# ADUCEDKConfig.cmake
#
# CMake package configuration file for the ADU Extension Development Kit.
#
# Provides the imported target ADUCEDK::edk which consumers can link
# against to get the correct include paths and compile definitions.
#
# Usage:
#   find_package(ADUCEDK REQUIRED)
#   target_link_libraries(my_extension PRIVATE ADUCEDK::edk)

cmake_minimum_required(VERSION 3.14)

if(NOT TARGET ADUCEDK::edk)
    add_library(ADUCEDK::edk INTERFACE IMPORTED)
    set_target_properties(ADUCEDK::edk PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/../../../include/aduc"
    )
endif()

set(ADUCEDK_FOUND TRUE)
