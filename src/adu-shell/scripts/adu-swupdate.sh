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

print_help() {
    echo "adu-swupdate.sh [-a] [-h] [-i image_file] [-l log_dir] [-r] "
    echo "-a                Applies the install by telling the bootloader to boot to the updated partition."
    echo "-h                Show this help message."
    echo "-i image_file     The update image file to install. For swupdate this is typically a .swu file"
    echo "-l log_dir        The folder where logs should be written."
    echo "-r                Reverts the apply by telling the bootloader to boot into the current partition."
}

action=""
num_actions=0
log_dir="/tmp"

while getopts "ahi:l:r" opt; do
    case "$opt" in
    a)
        action="apply"
        num_actions=$((num_actions + 1))
        ;;
    h)
        print_help
        $ret 0
        ;;
    i)
        action="install"
        image_file=$OPTARG
        num_actions=$((num_actions + 1))
        ;;
    l)
        log_dir=$OPTARG
        ;;
    r)
        action="revert"
        num_actions=$((num_actions + 1))
        ;;
    \?)
        print_help
        $ret 1
        ;;
    esac
done

# Check that an option was specified
if [[ $num_actions == 0 ]]; then
    echo "Must specify one action command line option."
    print_help
    $ret 1
fi

# Check that only one option was specified
if [[ $num_actions -gt 1 ]]; then
    echo "Must specify only one action command line option."
    print_help
    $ret 1
fi

# Make sure the log directory is created.
mkdir -p "$log_dir"

# SWUpdate doesn't support everything necessary for the dual-copy or A/B update strategy.
# Here we figure out the current OS partition and then set some environment variables
# that we use to tell swupdate which partition to target.
rootfs_dev=$(mount | grep "on / type" | cut -d':' -f 2 | cut -d' ' -f 1)
if [[ $rootfs_dev == '/dev/mmcblk0p2' ]]; then
    selection="stable,copy2"
    current_part=2
    update_part=3
else
    selection="stable,copy1"
    current_part=3
    update_part=2
fi

if [[ $action == "apply" ]]; then
    # Set the bootloader environment variable
    # to tell the bootloader to boot into the current partition
    # instead of the one that was updated.
    # rpipart variable is specific to our boot.scr script.
    echo "Applying update." >> "${log_dir}/swupdate.log"
    fw_setenv rpipart $update_part
    $ret $?
fi

if [[ $action == "revert" ]]; then
    # Set the bootloader environment variable
    # to tell the bootloader to boot into the current partition
    # instead of the one that was updated.
    # rpipart variable is specific to our boot.scr script.
    echo "Reverting update." >> "${log_dir}/swupdate.log"
    fw_setenv rpipart $current_part
    $ret $?
fi

if [[ $action == "install" ]]; then
    echo "Installing update." >> "${log_dir}/swupdate.log"
    if [[ -f $image_file ]]; then
        # Swupdate will use a public key to validate the signature of an image.
        # Here is how we generated the private key for signing the image
        # and how we generated that public key file used to validate the image signature.
        # Generated RSA private key with password using command:
        # openssl genrsa -aes256 -passout file:priv.pass -out priv.pem
        # Generated RSA public key from private key using command:
        # openssl rsa -in ${WORKDIR}/priv.pem -out ${WORKDIR}/public.pem -outform PEM -pubout

        # Call swupdate with the image file and the public key for signature validation
        swupdate -v -i "${image_file}" -k /adukey/public.pem -e ${selection} &>> "${log_dir}/swupdate.log"
        $ret $?
    else
        echo "Image file $image_file was not found." >> "${log_dir}/swupdate.log"
        $ret 1
    fi
fi
