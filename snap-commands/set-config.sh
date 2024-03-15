#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

CONFIG_FILE=""
BASE64_DATA=""
COMMAND_NAME="deviceupdate-agent.set-config"

function show_usage() {

cat << EOS
Usage: $(basename $0) [OPTIONS]

Description:
  This script decodes and saves base64 encoded configuration data to a file with the given name in the $SNAP_DATA/config directory.
  The script takes two required options - the name of the config file and the base64 encoded data - and saves the data to the specified file.
  Only two config file names are allowed: "du-config.json" and "du-diagnostics-config.json".

Options:
  -c, --config-file <config file name>   Specifies the name of the configuration file to be created or updated.
                                         The only valid values for the config file name are "du-config.json" and "du-diagnostics-config.json".
                                         This option is required.

  -d, --data <base64 encoded data>       Specifies the base64 encoded configuration data to be saved to the specified config file.
                                         This option is required.

  -h, --help                             Displays this usage information.

Examples:
  Save the base64 encoded configuration data to the "du-config.json" file:

    $ sudo snap $COMMAND_NAME --config-file du-config.json --data "ABCDEF=="

  Save the base64 encoded configuration data to the "du-diagnostics-config.json" file:

    $ sudo snap $COMMAND_NAME -c du-diagnostics-config.json -d "UVWXYZ=="
EOS
}

while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    -c|--config-file)
      CONFIG_FILE="$2"
      shift
      shift
      ;;

    -d|--data)
      BASE64_DATA="$2"
      shift
      shift
      ;;

    -h|--help)
      show_usage
      exit 0
      ;;

    *)
      echo "Invalid option: $key"
      exit 1
    ;;
esac
done

if [[ -z "$CONFIG_FILE" || -z "$BASE64_DATA" ]]; then
  show_usage
  exit 1
fi

case $CONFIG_FILE in
  du-config.json|du-diagnostics-config.json)
    ;;
  *)
    echo -e "Invalid config file name: $CONFIG_FILE\n"
    show_usage
    exit 1
    ;;
esac

dst_file="$SNAP_DATA/config/$CONFIG_FILE"

if echo "$BASE64_DATA" | base64 --decode > "$dst_file"; then
  echo "Configuration data saved to $dst_file."
else
  echo "An error occurred while saving the data."
  exit 1
fi
