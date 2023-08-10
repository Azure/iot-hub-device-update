# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.5)

# Sets a cache variable with the following precedence
# 1. Value already present in cache. This macro will not override an existing cache value.
# 2. An environment variable with the same name. Only reads from the environment and does not set environment variables.
# 3. The specified default value.
macro (
    set_cache_with_env_or_default
    variable
    default
    type
    description)
    if (DEFINED ENV{${variable}})
        set (
            ${variable}
            $ENV{${variable}}
            CACHE ${type} ${description})
    else ()
        set (
            ${variable}
            ${default}
            CACHE ${type} ${description})
    endif ()
endmacro ()

# Finds and copies all test data files from the source tree folder to output path
#
# Parameters:
# base_dir - the base directory absolute path from which to find test data paths
# test_data_path_segment - the path segment, e.g. testdata of test data dirs
# output_dir - absolute output path base directory to put testdata files
#
# For example, if base_dir is $SRC, test_data_path_segment is "testdata", and output path is /var/adu/out, then
# if the following exist
#     $SRC/.../foo/tests /testdata /testsuite1/testfile1.json
#                                             testfile2.json
#
#     $SRC/.../bar/tests /testdata /testsuite2/testfile1.json
#                                             testfile2.txt
#
# then the files would get copied as follows:
#                     /var/adu/out /testsuite1/testfile1.json
#                                              testfile2.json
#
#                     /var/adu/out /testsuite2/testfile1.json
#                                              testfile2.txt
#
macro (
    copy_test_data
    base_dir
    test_data_path_segment
    output_dir)

    message (STATUS "Copying test data from '${base_dir}' to '${output_dir}'")

    file (
        GLOB_RECURSE file_list
        LIST_DIRECTORIES true
        RELATIVE ${base_dir}
        ${base_dir}/*)
    foreach (child ${file_list})
        if (child MATCHES "\/testdata\/(.*)$")
            set (TARGET_DIR "${output_dir}/${CMAKE_MATCH_1}")
            if (IS_DIRECTORY ${base_dir}/${child})
                file (MAKE_DIRECTORY ${TARGET_DIR})
            else ()
                file (
                    COPY_FILE
                    "${base_dir}/${child}"
                    ${TARGET_DIR}
                    RESULT
                    result)
                if (NOT
                    result
                    EQUAL
                    "0")
                    message (FATAL_ERROR "COPY_FILE failed: ${result}")
                endif ()
            endif ()
        endif ()
    endforeach ()

endmacro ()
