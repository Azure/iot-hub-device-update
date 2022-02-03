# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Wrapper around the existing FindCURL module provided by CMake.
# Adds target CURL::libcurl if it doesn't already exist.
# FindCURL doesn't export targets until CMake 3.12.

cmake_minimum_required (VERSION 3.5)

macro (find_curl)
    find_package (CURL
                  ${ARGV})

    if (CURL_FOUND AND NOT TARGET CURL::libcurl)
        add_library (CURL::libcurl
                     INTERFACE
                     IMPORTED)
        set_target_properties (CURL::libcurl
                               PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                          "${CURL_INCLUDE_DIRS}"
                                          INTERFACE_LINK_LIBRARIES
                                          "${CURL_LIBRARIES}")
    endif ()
endmacro ()
