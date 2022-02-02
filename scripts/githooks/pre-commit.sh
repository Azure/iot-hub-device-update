#!/usr/bin/env bash

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

if ! GITROOT="$(git rev-parse --show-toplevel 2> /dev/null)"; then
    echo "'git rev-parse' failed with $?"
    $ret 1
fi

if [ -z "$GITROOT" ]; then
    echo 'Unable to determine git root.' >&2
    $ret 1
fi

# Redirect output to stderr.
exec 1>&2

# Run shell format first in case below scripts have issues
if ! "$GITROOT/scripts/sh-format.sh" -v; then
    $ret 1
fi

if ! "$GITROOT/scripts/clang-format.sh"; then
    $ret 1
fi

# Unfortunately, cmake-format cannot take in piped file contents, so
# not using it for pre-commit hook currently.
# if ! "$GITROOT/scripts/cmake-format.sh"; then
#     $ret 1
# fi
