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

# To force all cmakefiles to be reformatted, try:
# find $(git rev-parse --show-toplevel) -name CMakeLists.txt -exec sh -c "echo "" >> {}" \;
# then run this script.

if ! [ -x "$(command -v git)" ]; then
    error 'git is not installed.'
    $ret 1
fi

if ! [ -x "$(command -v cmake-format)" ]; then
    error 'cmake-format is not installed. Try: sudo --set-home pip3 install cmake-format'
    $ret 1
fi

GITROOT="$(git rev-parse --show-toplevel 2> /dev/null)"

if [ -z "$GITROOT" ]; then
    error 'Unable to determine git root.'
    $ret 1
fi

pushd "$GITROOT" > /dev/null || $ret
# diff-filter=d will exclude deleted files.
IFS=$'\n'
cmake_format_failed_or_reformatted=false
# Use --cached to only check the staged files before commit.
for FILE in $(git diff --diff-filter=d --relative --name-only --cached HEAD -- "CMakeLists.txt" "*/CMakeLists.txt" "*.cmake" "*/*.cmake"); do
    echo Formatting "$FILE"
    # Unfortunately, cmake-format doesn't work when piping git cat-file blob
    # into it so that it processes only staged file contents, but we use the
    # --check argument so that a reformatting results in non-zero exit code.
    if ! cmake-format --check "$FILE" 2> /dev/null; then
        cmake_format_failed_or_reformatted=true

        # now, in-place format the file. Unfortunately, it will be in the
        # unstaged working set, so we notify the user farther down.
        cmake-format -i "$FILE"
    fi
done
IFS=' '
popd > /dev/null || $ret

if [[ $cmake_format_failed_or_reformatted == "true" ]]; then
    error 'cmake-format failed or had to reformat a file. Fix the issues and/or git add the reformatted file(s).'
    $ret 1
fi

$ret 0
