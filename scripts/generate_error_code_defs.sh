#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Ensure that getopt starts from first option if ". <script.sh>" was used.
OPTIND=1

# Ensure we dont end the user's terminal session if invoked from source (".").
if [[ $0 != "${BASH_SOURCE[0]}" ]]; then
    ret='return'
else
    ret='exit'
fi

error() { echo -e "\033[1;31mError:\033[0m $*" >&2; }

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" > /dev/null 2>&1 && pwd)"

result_generator_dir_path=$script_dir/error_code_generator_defs

json_file_path=""
result_file_path=""
print_help=false
skip_clang_format=false

print_help() {
    python3 "$result_generator_dir_path/error_code_defs_generator.py" -h
}

while [[ $1 != "" ]]; do
    case $1 in
    -j | --json-file-path)
        shift
        json_file_path=$1
        ;;
    -r | --result-file-path)
        shift
        result_file_path=$1
        ;;
    -h | --help)
        shift
        print_help=true
        ;;
    -s | --skip-clang-format)
        shift
        skip_clang_format=true
        ;;
    *)
        error "Invalid argument: $1"
        $ret 1
        ;;
    esac
    shift
done

if [[ $print_help == "true" ]]; then
    print_help
    $ret 0
fi

if [ "$json_file_path" == "" ]; then
    error "Must pass a file path to the json file defining the result codes"
    print_help
    $ret 1
fi

if [ "$result_file_path" == "" ]; then
    error "Must pass directory to which the error code define file must be passed"
    print_help
    $ret 1
fi

# Make sure that result_h_generator.py is executable
chmod u+x "$result_generator_dir_path/error_code_defs_generator.py"

python3 "$result_generator_dir_path/error_code_defs_generator.py" -j "$json_file_path" -r "$result_file_path"

python_ret=$?

if [[ $python_ret != 0 ]]; then
    # error "Failed to generate error code definition file"
    $python_ret 1
fi

if [ $skip_clang_format == "false" ]; then
    clang-format -verbose -style=file -i "$result_file_path"
fi

$ret 0
