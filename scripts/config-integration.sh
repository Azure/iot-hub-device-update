#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Ensure we dont end the user's terminal session if invoked from source (".").
if [[ $0 != "${BASH_SOURCE[0]}" ]]; then
    ret='return'
else
    ret='exit'
fi

error() { echo -e "\033[1;31mError:\033[0m $*" >&2; }
warn() { echo -e "\033[1;33mWarning:\033[0m $*" >&2; }

current_schema_version="1.1"

conf_file=$1
new_conf_file="/etc/adu/du-config.json"

if [ ! -f "$conf_file" ]; then
    error "Could not find the existing conf file"
    $ret 1
fi

connection_string=$(grep "^connection_string\s*=\s*" "$conf_file" | sed -e "s/^connection_string\s*=\s*//")

if [ ! "$connection_string" ]; then
    error "File does not contain connection_string"
    $ret 1
fi

aduc_manufacturer=$(grep "^aduc_manufacturer\s*=\s*" "$conf_file" | sed -e "s/^aduc_manufacturer\s*=\s*//")
aduc_model=$(grep "^aduc_model\s*=\s*" "$conf_file" | sed -e "s/^aduc_model\s*=\s*//")
manufacturer=$(grep "^manufacturer\s*=\s*" "$conf_file" | sed -e "s/^manufacturer\s*=\s*//")
model=$(grep "^model\s*=\s*" "$conf_file" | sed -e "s/^model\s*=\s*//")

json_content=$(
    cat << END_OF_JSON
{
  "schemaVersion": "$current_schema_version",
  "aduShellTrustedUsers": [
    "adu",
    "do"
  ],
  "iotHubProtocol": "mqtt",
  <% MANUFACTURER %>
  <% MODEL %>
  "agents": [
    {
      "name": "host-update",
      "runas": "adu",
      "connectionSource": {
        "connectionType": "string",
        "connectionData": "$connection_string"
      }<% ADUC_MANUFACTURER %><% ADUC_MODEL %>
    }
  ]
}
END_OF_JSON
)

if [[ "$manufacturer" ]]; then
    manufacturer_json=$(
        cat << EOM
"manufacturer": "$manufacturer",
EOM
    )
    json_content=${json_content/<% MANUFACTURER %>/$manufacturer_json}
else
    warn "DeviceInfo manufacturer is mandatory, please add it in $new_conf_file"
    no_manufacturer_json=$(
        cat << EOM
"manufacturer": <INSERT DEVICE INFO MANUFACTURER>,
EOM
    )
    json_content=${json_content/<% MANUFACTURER %>/$no_manufacturer_json}
fi

if [[ "$model" ]]; then
    model_json=$(
        cat << EOM
"model": "$model",
EOM
    )
    json_content=${json_content/<% MODEL %>/$model_json}
else
    warn "DeviceInfo model is mandatory, please add it in $new_conf_file"
    no_model_json=$(
        cat << EOM
"model": <INSERT DEVICE INFO MODEL>,
EOM
    )
    json_content=${json_content/<% MODEL %>/$no_model_json}
fi

if [[ "$aduc_manufacturer" ]]; then
    aduc_manufacturer_json=$(
        cat << EOM
,
      "manufacturer": "$aduc_manufacturer"
EOM
    )
    processed_aduc_manufacturer_json=$(echo "$aduc_manufacturer_json" | tr '\n' '#' | sed -e 's/#$//g')
    json_content=$(echo "$json_content" | sed -e "s/<% ADUC_MANUFACTURER %>/$processed_aduc_manufacturer_json/g;s/#/\n/g")
else
    warn "DeviceProperty manufacturer (aduc_manufacturer) is mandatory, please add it in $new_conf_file"
    no_aduc_manufacturer_json=$(
        cat << EOM
,
      "manufacturer": <INSERT DEVICE PROPERTY MANUFACTURER>
EOM
    )
    json_content=${json_content/<% ADUC_MANUFACTURER %>/$no_aduc_manufacturer_json}
fi

if [[ "$aduc_model" ]]; then
    aduc_model_json=$(
        cat << EOM
,
      "model": "$aduc_model"
EOM
    )
    processed_aduc_model_json=$(echo "$aduc_model_json" | tr '\n' '#' | sed -e 's/#$//g')
    json_content=$(echo "$json_content" | sed -e "s/<% ADUC_MODEL %>/$processed_aduc_model_json/g;s/#/\n/g")
else
    warn "DeviceProperty model (aduc_model) is mandatory, please add it in $new_conf_file"
    no_aduc_model_json=$(
        cat << EOM
,
      "model": <INSERT DEVICE PROPERTY MODEL>
EOM
    )
    json_content=${json_content/<% ADUC_MODEL %>/$no_aduc_model_json}
fi

echo "$json_content" | sudo tee $new_conf_file || {
    error "Failed to write to the new configuration file"
}
