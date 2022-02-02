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

issue_with_file=false
pushd "$GITROOT" > /dev/null || $ret
# diff-filter=d will exclude deleted files.
# --cached for only the staged files
IFS=$'\n'
for FILE in $(git diff --diff-filter=d --relative --name-only --cached HEAD -- "*.[CcHh]" "*.[CcHh][Pp][Pp]"); do
    # Use --dry-run and --Werror to have clang-format return non-zero error return if it
    # had to reformat a file.
    # Need to use git cat-file blob so that it gets staged file in case it got formatted in working set, which would
    # allow it to bypass pre-commit hook even if staged file is not formatted correctly.
    if ! git cat-file blob :"$FILE" 2>&1 | clang-format --dry-run --Werror --verbose --style=file; then
        issue_with_file=true
        clang-format --verbose --style=file -i "$FILE"
    fi
done
IFS=' '
popd > /dev/null || $ret

if [[ $issue_with_file == "true" ]]; then
    echo 'Error: Some staged C/C++ files had issues reformatted. Fix issues and/or git add the reformatted changes.'
    $ret 1
fi

$ret 0
