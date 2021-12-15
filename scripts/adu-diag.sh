# !/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Ensure we dont end the user's terminal session if invoked from source (".").
if [[ $0 != "${BASH_SOURCE[0]}" ]]; then
    ret=return
else
    ret=exit
fi

error() { echo -e "\033[1;31mError:\033[0m $*" >&2; }

# Copy ADUC related diagnostic files to current directory

gather_aduc_info() {
    {
        # ADUC_CONF_FOLDER

        cp /etc/adu/* .

        # ADUC_DATA_FOLDER

        cp /var/lib/adu/* .

        # ADUC_LOG_FOLDER

        cp /var/log/adu/* .

        # Agent info.

        cp /usr/lib/systemd/system/adu-agent.service .

        systemctl status adu-agent.service > adu-agent.service.txt

        ls -l /usr/bin/AducIotAgent > AducIotAgent.txt
    } &>> adu-diag.log
}

gather_do_info() {
    {
        cp /var/cache/deliveryoptimization-agent/log/* .
        cp /tmp/doclient-apt-plugin.log .
        systemctl status deliveryoptimization-agent > deliveryoptimization-agent.service.txt
    } &>> adu-diag.log
}

temp_dir="$(mktemp -d /tmp/adu-diag.XXXXXX)"

if [[ ! $temp_dir || ! -d $temp_dir ]]; then
    error "Could not create temp dir"
    $ret 1
fi

function cleanup() {
    rm -rf "$temp_dir"
}

trap cleanup EXIT

pushd "$temp_dir" || {
    error "pushd command failed."
    $ret 1
}

gather_aduc_info

gather_do_info

tar -czvf ~/adu-diag.tar.gz . || {
    error "tar command failed."
    popd
    $ret 1
}

popd

echo ""

echo 'ADU Diag file created: '

ls -l ~/adu-diag.tar.gz
