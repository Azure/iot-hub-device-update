#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Ensure that getopt starts from first option if ". <script.sh>" was used.
OPTIND=1

# Ensure we dont end the user's terminal session if invoked from source (".").
if [[ $0 != "${BASH_SOURCE[0]}" ]]; then
    ret=return
else
    ret=exit
fi

error() { echo -e "\033[1;31mError:\033[0m $*" >&2; }

validate=false

print_help() {
    echo "Usage: sh-format.sh [options...]"
    echo "-v, --validate        Validates shell scripts as well as format."
    echo "-h, --help            Show this help message."
}

while [[ $1 != "" ]]; do
    case $1 in
    -v | --validate)
        validate=true
        ;;
    -h | --help)
        print_help
        $ret 0
        ;;
    *)
        error "Invalid argument: $*"
        $ret 1
        ;;
    esac
    shift
done

if ! [ -x "$(command -v git)" ]; then
    error 'git is not installed. Try: apt install git'
    $ret 1
fi

if ! [ -x "$(command -v shfmt)" ]; then
    error 'shfmt is not installed. Try: snap install shfmt'
    $ret 1
fi

GITROOT="$(git rev-parse --show-toplevel 2> /dev/null)"

if [ -z "$GITROOT" ]; then
    error 'Unable to determine git root.'
    $ret 1
fi

pushd "$GITROOT" > /dev/null
# diff-filter=d will exclude deleted files.
IFS=$'\n'
shell_files=$(git diff --diff-filter=d --relative --name-only HEAD -- "*.sh" "*.bash" "*.mksh")
for FILE in $shell_files; do
    echo "Formatting $FILE"
    shfmt -s -w -i 4 -bn -sr "$FILE"
done

if [[ $validate == "true" ]]; then
    if ! [ -x "$(command -v shellcheck)" ]; then
        echo 'Error: shellcheck is not installed. Try: apt install shellcheck' >&2
        $ret 1
    fi

    for FILE in $shell_files; do
        echo "Checking $FILE"
        shellcheck -s bash "$FILE"
    done
fi
IFS=' '
popd > /dev/null
