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

# To force all cmakefiles to be reformatted, try:
# find $(git rev-parse --show-toplevel) -name CMakeLists.txt -exec sh -c "echo "" >> {}" \;
# then run this script.

if ! [ -x "$(command -v git)" ]; then
    echo 'Error: git is not installed.' >&2
    $ret 1
fi

if ! [ -x "$(command -v cmake-format)" ]; then
    echo 'Error: cmake-format is not installed. Try: sudo --set-home pip3 install cmake-format' >&2
    $ret 1
fi

GITROOT="$(git rev-parse --show-toplevel 2> /dev/null)"

if [ -z "$GITROOT" ]; then
    echo 'Unable to determine git root.' >&2
    $ret 1
fi

pushd "$GITROOT" > /dev/null
# diff-filter=d will exclude deleted files.
IFS=$'\n'
for FILE in $(git diff --diff-filter=d --relative --name-only HEAD -- "CMakeLists.txt" "*/CMakeLists.txt" "*.cmake" "*/*.cmake"); do
    echo Formatting "$FILE"
    cmake-format -i "$FILE"
done
IFS=' '
popd > /dev/null
