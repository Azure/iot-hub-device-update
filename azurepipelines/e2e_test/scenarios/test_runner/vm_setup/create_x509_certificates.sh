#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# install-deps.sh makes it more convenient to install
# dependencies for ADU Agent and Delivery Optimization.
# Some dependencies are installed via packages and
# others are installed from source code.

# Ensure that getopt starts from first option if ". <script.sh>" was used.
OPTIND=1

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" > /dev/null 2>&1 && pwd)"

# Ensure we dont end the user's terminal session if invoked from source (".").
if [[ $0 != "${BASH_SOURCE[0]}" ]]; then
    ret='return'
else
    ret='exit'
fi

# Use sudo if user is not root
SUDO=""
if [ "$(id -u)" != "0" ]; then
    SUDO="sudo"
fi

warn() { echo -e "\033[1;33mWarning:\033[0m $*" >&2; }

error() { echo -e "\033[1;31mError:\033[0m $*" >&2; }

#defaults
device_id=""
output_directory=""
work_directory="x509_provisioning_work_dir"
iotedge_repo_link="git clone https://github.com/Azure/iotedge.git"

print_help() {
    echo "Usage: create_x509_device.sh [options...]"
    echo "-d, --device-id <device_id>      Device ID to use for the device."
    echo "-p, --output-dir-path <path>     Fully actualized path to output directory for the"
    echo "                                 device certificate, key, and provisioning configurations to"
    echo "-h, --help                       Print this help message."
}

while [[ $1 != "" ]]; do
    case $1 in
    -d | --device-id)
        shift
        device_id=$1
        ;;
    -p | --output-path)
        shift
        output_directory=$1
        ;;
    -h | --help)
        print_help
        exit 0
        ;;
    *)
        echo "Invalid argument: $*"
        print_help
        exit 1
        ;;
    esac
    shift
done

if [[ -z $device_id ]]; then
    error "Device ID is required."
    print_help
    $ret 1
fi

if [[ -z $output_directory ]]; then
    error "Output directory path is required."
    print_help
    $ret 1
fi

# Clone the repo to be used for generating the device certificate
# By end of function $work_directory/${device_id}-primary.pem
# and ${work_directory}/${device_id}-secondary.pem exist and can be worked on
do_create_certificates() {
    if [[ -d "${script_dir}/${work_directory}" ]]; then
        $SUDO rm -rf "${script_dir}/${work_directory}" || $ret
    fi

    echo "Creating the output directory"
    $SUDO mkdir -p "${script_dir}/${work_directory}" || $ret

    pushd $work_directory || $ret

    echo "Cloning the iotedge repo"
    git clone "${iotedge_repo_link}"

    echo "Copying the files needed to create the certificates"

    $SUDO cp ./iotedge/tools/CACertificates/*.cnf . || $ret

    $SUDO cp ./iotedge/tools/CACertificates/certGen.sh . || $ret

    $SUDO chmod +x ./certGen.sh || $ret

    $SUDO ./certGen.sh create_root_and_intermediate || $ret

    $SUDO ./certGen.sh create_device_certificate "${device_id}" || $ret

    echo "Creating the certificates"
    $SUDO ./certGen.sh create_device_certificate "${device_id}-primary" || $ret

    $SUDO ./certGen.sh create_device_certificate "${device_id}-secondary" || $ret

    echo "Copying the certificates from ${script_dir}/${work_directory}/certs to ${output_directory}"

    # Copy over the certificates to the output directory
    $SUDO cp "./certs/iot-device-${device_id}-primary.cert.pem" "${output_directory}/${device_id}-primary.pem" || $ret
    $SUDO cp "./certs/iot-device-${device_id}-secondary.cert.pem" "${output_directory}/${device_id}-secondary.pem" || $ret

    echo "Copying the keys from ${script_dir}/${work_directory}/private to ${output_directory}"
    $SUDO cp "./private/iot-device-${device_id}-primary.key.pem" "${output_directory}/${device_id}-primary-key.pem" || $ret
    $SUDO cp "./private/iot-device-${device_id}-secondary.key.pem" "${output_directory}/${device_id}-secondary-key.pem" || $ret

    popd || $ret

    echo "Cleaning out work directory..."
    $SUDO rm -rf "${script_dir}/${work_directory}" || $ret
}

do_extract_thumbprints() {

    echo "Extracting the thumbprints from the certificates"
    $SUDO openssl x509 -in "${output_directory}/${device_id}-primary.pem" -noout -sha256 -fingerprint | sed 's/[:]//g' > "${output_directory}/${device_id}-primary-thumbprint.txt" || $ret

    $SUDO openssl x509 -in "${output_directory}/${device_id}-secondary.pem" -noout -sha256 -fingerprint | sed 's/[:]//g' > "${output_directory}/${device_id}-primary-thumbprint.txt" || $ret
}

do_create_certificates || $ret

do_extract_thumbprints || $ret
