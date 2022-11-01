#!/usr/bin/env bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# The absolute path to the dir where this script is located
script_dir=''

#
# MAIN
#

# When calling this with arguments (creating bundle not applying bootstrap), then not as root/sudo, caller must call:
#
# sudo apt-get install python3 python3-pip
#
# and
#
# pip3 install jsonschema
# pip3 install requests

if [[ $(whoami) != 'root' ]]; then
    echo "This needs to be run as root. Try: sudo $0"
    exit 1
fi

# Get the abs path to the dir where script is located
pushd "$(dirname "$0")" > /dev/null || exit
script_dir="$(pwd)"
popd > /dev/null || exit

# Pass-thru args to bootstrap.py
"${script_dir}/bootstrap.py" "$@"
