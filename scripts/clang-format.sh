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

if ! [ -x "$(command -v git)" ]; then
    echo 'Error: git is not installed.' >&2
    $ret 1
fi

if ! [ -x "$(command -v clang-format)" ]; then
    echo 'Error: clang-format is not installed. Try: apt install clang-format' >&2
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
for FILE in $(git diff --diff-filter=d --relative --name-only HEAD -- "*.[CcHh]" "*.[CcHh][Pp][Pp]"); do
    clang-format -verbose -style=file -i "$FILE"
done
IFS=' '
popd > /dev/null
