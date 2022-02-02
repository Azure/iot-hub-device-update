#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Script to debug ADU agent.

# Ensure that getopt starts from first option if ". <script.sh>" was used.
OPTIND=1

# Ensure we dont end the user's terminal session if invoked from source (".").
if [[ $0 != "${BASH_SOURCE[0]}" ]]; then
    ret='return'
else
    ret='exit'
fi

usage() {
    echo "Usage: debug_agent.sh [OPTION] connection_string"
    echo ""
    echo "  connection_string        SAS connection string."
    echo "  -a                       Specify location of aduciotagent."
    echo "  -n                       Runs agent without debugger attached."
    echo "  -v                       Run agent under valgrind."
    echo "  -h                       Show this help message."
}

aduciotagent=out/bin/AducIotAgent
no_debugger=false
valgrind=false

while getopts ":a:hnv" option; do
    case "$option" in
    a)
        aduciotagent=$OPTARG
        ;;
    h)
        usage
        $ret 1
        ;;
    n)
        no_debugger=true
        ;;
    v)
        valgrind=true
        ;;
    \?)
        echo "Unknown option: -$OPTARG" >&2
        $ret 1
        ;;
    :)
        echo "Missing option argument for -$OPTARG" >&2
        $ret 1
        ;;
    *)
        echo "Unimplemented option: -$OPTARG" >&2
        $ret 1
        ;;
    esac
done

shift $((OPTIND - 1))

if (($# == 0)); then
    echo "Connection string not specified."
    $ret 1
fi

sas=$1
shift

if [ "$valgrind" = true ]; then
    # Can also run under gdb remote using: valgrind --vgdb=yes --vgdb-error=0 $aduciotagent
    valgrind --tool=memcheck --leak-check=full --error-exitcode=1 --log-file=/tmp/valgrind.log --show-reachable=yes --num-callers=50 --track-fds=yes "$aduciotagent" "$sas" "$@"
    retVal=$?
    if [ $retVal -ne 0 ]; then
        printf "\033[0;31mMemory leak(s) detected!\033[0m\n"
        cat /tmp/valgrind.log
    fi
    $ret 0
fi

if [ "$no_debugger" = true ]; then
    "$aduciotagent" "$sas" "$@"
else
    gdb --tui --quiet --eval-command="set print thread-events off" --args "$aduciotagent" "$sas" "$@"
fi

$ret 0
