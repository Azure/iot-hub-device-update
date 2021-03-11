# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Find cmake module for the parson library and header.
# Exports Parson::parson target

cmake_minimum_required (VERSION 3.5)

include (FindPackageHandleStandardArgs)

find_path (Parson_INCLUDE_DIR
           NAMES parson.h
           PATH_SUFFIXES azureiot
                         azureiot/inc)

find_library (Parson_LIBRARY
              parson)

find_package_handle_standard_args (Parson
                                   DEFAULT_MSG
                                   Parson_INCLUDE_DIR
                                   Parson_LIBRARY)

if (Parson_FOUND)
    set (Parson_LIBRARIES ${Parson_LIBRARY})
    set (Parson_INCLUDE_DIRS ${Parson_INCLUDE_DIR})

    if (NOT TARGET Parson::parson)
        add_library (Parson::parson
                     INTERFACE
                     IMPORTED)
        set_target_properties (Parson::parson
                               PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                          "${Parson_INCLUDE_DIRS}"
                                          INTERFACE_LINK_LIBRARIES
                                          "${Parson_LIBRARIES}")
    endif ()
endif ()
