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
# For example, if base_dir is $SRC, test_data_path_segment is "testdata", and output path is /var/adu/testdata, then
# if the following exist
#     $SRC/.../foo/tests/testdata/testsuite1/testdata1.json
#                                            testdata2.json
#
#     $SRC/.../bar/tests/testdata/testsuite2/testdata1.json
#                                            testdata2.txt
#
# then the files would get copied as follows:
#     /var/adu/testdata/testsuite1/testdata1.json
#                                  testdata2.json
#
#     /var/adu/testdata/testsuite2/testdata1.json
#                                  testdata2.txt
#
macro (
    copy_test_data
    base_dir
    test_data_path_segment
    output_dir)

    string (CONCAT PATH_PATTERN "*/" ${test_data_path_segment} "/*")
    execute_process(
        COMMAND find "${base_dir}" -path ${PATH_PATTERN} -name *.* -print
        OUTPUT_VARIABLE EXEC_OUTPUT_TEST_DATA_FILES
        OUTPUT_STRIP_TRAILING_WHITESPACE
        WORKING_DIRECTORY ${base_dir}
        #COMMAND_ECHO STDOUT
    )

    string(REPLACE "\n" ";" TEST_DATA_FILE_LIST ${EXEC_OUTPUT_TEST_DATA_FILES})

    foreach (test_data_file_path ${TEST_DATA_FILE_LIST})

        execute_process(
            COMMAND echo ${test_data_file_path}
            COMMAND sed -e "s\/^.\\+${test_data_path_segment}\\/\/\/"
            OUTPUT_VARIABLE FILE_PATH_AFTER_TEST_DATA_PATH_SEGMENT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            WORKING_DIRECTORY ${base_dir}
            #COMMAND_ECHO STDOUT
        )

        execute_process(
            COMMAND dirname ${output_dir}/${FILE_PATH_AFTER_TEST_DATA_PATH_SEGMENT}
            OUTPUT_VARIABLE DIR_PATH_AFTER_TEST_DATA_PATH_SEGMENT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            WORKING_DIRECTORY ${base_dir}
            #COMMAND_ECHO STDOUT
        )

        execute_process(
            COMMAND mkdir -p ${DIR_PATH_AFTER_TEST_DATA_PATH_SEGMENT}
            WORKING_DIRECTORY ${base_dir}
            #COMMAND_ECHO STDOUT
        )

        execute_process(
            COMMAND cp ${test_data_file_path} ${DIR_PATH_AFTER_TEST_DATA_PATH_SEGMENT}/
            WORKING_DIRECTORY ${base_dir}
            #COMMAND_ECHO STDOUT
        )

    endforeach()

endmacro ()
