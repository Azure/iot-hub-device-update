#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# install-openssl.sh makes it more convenient to install openssl or for customers to upgrade

# Ensure that getopt starts from first option if ". <script.sh>" was used.
OPTIND=1

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

# Ensure we dont end the user's terminal session if invoked from source (".").
if [[ $0 != "${BASH_SOURCE[0]}" ]]; then
    ret='return'
else
    ret='exit'
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" > /dev/null 2>&1 && pwd)"

image_generator_git_repo="https://github.com/nihemstr/ImageUpdateExample.git"
multi_partition_image_branch="multi-partition-image"
update_image_branch="update-image"

aduc_packages=('p7zip-full' 'jq' 'qemu-user-static' 'cpio')
print_help() {
    echo "Usage: x509_vm_setup.sh [OPTIONS]"
    echo "Options:"
    echo "  -d, --distro       Distribution and version (ubuntu-18.04, ubuntu-20.04)"
    echo "                     Please note only ubuntu-18.04 and ubuntu-20.04 are supported"
    echo "  -w, --workspace    Workspace directory to use for generating all of the files"
    echo "  -o, --out          Output directory to place the generated images and update files"
}

distro=""
workspace="/tmp/deviceupdate-image-creator"
out=""

base_module_path="adu-image/src"
image_copy_path="$base_module_path/image/"
config_copy_path="$base_module_path/config/"
build_command_path="$base_module_path/build_dist"

update_module_path="ubuntu-img/src"
update_copy_path="$update_module_path/image/"
update_config_copy_path="$update_module_path/config/"
update_build_command_path="$update_module_path/build_dist"

while [[ $1 != "" ]]; do
    case $1 in
    -d | --distro)
        shift
        distro=$1
        ;;
    -w | --workspace)
        shift
        workspace=$1
        ;;
    -o | --out)
        shift
        out=$1
        ;;
    *)
        echo "Invalid argument: $*"
        print_help
        exit 1
        ;;
    esac
    shift
done

if [[ $distro == "" ]]; then
    echo "Please specify a distro"
    print_help
    exit 1
fi

if [[ $out == "" ]]; then
    echo "Please an output directory"
    print_help
    exit 1
fi

# Install the packages we need to run the thing
$SUDO apt-get install --yes "${aduc_packages[@]}" || return
# Start with cloning the repository and building the two seperate images

if [ ! -d "$workspace" ]; then
    mkdir -p "$workspace" || $ret 1
fi

pushd "$workspace" || $ret 1

# Checking out the initial branch so we can generate the image to be flashed
git clone "$multi_partition_image_branch" "$image_generator_git_repo" "$workspace/image_to_flash" || $ret 1

pushd "$workspace/image_to_flash" || $ret 1

# Get the base image we're going to use for creating the image
base_image=""
if [[ $distro == "ubuntu-18.04" ]]; then
    wget "https://cdimage.ubuntu.com/releases/18.04/release/ubuntu-18.04.5-preinstalled-server-arm64+raspi.img.xz" || $ret 1
    base_image="ubuntu-18.04.5-preinstalled-server-arm64+raspi.img.xz"
elif [[ $distro == "ubuntu-20.04" ]]; then
    wget "https://cdimage.ubuntu.com/releases/20.04/release/ubuntu-20.04.5-preinstalled-server-arm64+raspi.img.xz" || $ret 1
    base_image="ubuntu-20.04.5-preinstalled-server-arm64+raspi.img.xz"
else
    echo "Unsupported distro: $distro"
    print_help
    exit 1
fi

# Copy the image to the rgiht workspace
cp "$base_image" "$image_copy_path/$base_image" || $ret 1

# Copy in the config file for the distribution

if [[ $distro == "ubuntu-18.04" ]]; then
    cp "$script_dir/ubuntu-1804-multi-config" "workspace/image_to_flash/$config_copy_path" || $ret 1
elif [[ $distro == "ubuntu-20.04" ]]; then
    cp "$script_dir/ubuntu-2004-multi-config" "$workspace/image_to_flash/$config_copy_path" || $ret 1
else
    echo "Unsupported distro: $distro"
    print_help
    exit 1
fi

# Now we need to build the image
$SUDO "$workspace/image_to_flash/$build_command_path" || $ret 1

# Output image will be in the workspace area of the image_to_flash/adu-image/src/workspace/ directory

popd || $ret 1

# Now we need to checkout the update branch and build the update image
git clone "$update_image_branch" "$image_generator_git_repo" "$workspace/update_image" || $ret 1

pushd "$workspace/update_image" || $ret 1

# Get the base image we're going to use for creating the image
base_image=""
if [[ $distro == "ubuntu-18.04" ]]; then
    wget "https://cdimage.ubuntu.com/releases/18.04/release/ubuntu-18.04.5-preinstalled-server-arm64+raspi.img.xz" || $ret 1
    base_image="ubuntu-18.04.5-preinstalled-server-arm64+raspi.img.xz"
elif [[ $distro == "ubuntu-20.04" ]]; then
    wget "https://cdimage.ubuntu.com/releases/20.04/release/ubuntu-20.04.5-preinstalled-server-arm64+raspi.img.xz" || $ret 1
    base_image="ubuntu-20.04.5-preinstalled-server-arm64+raspi.img.xz"
else
    echo "Unsupported distro: $distro"
    print_help
    exit 1
fi

# Copy the image to the right workspace
cp "$base_image" "$update_copy_path/$base_image" || $ret 1

# Copy in the config file for the distribution

if [[ $distro == "ubuntu-18.04" ]]; then
    cp "$script_dir/ubuntu-1804-update-config" "./$update_config_copy_path" || $ret 1
elif [[ $distro == "ubuntu-20.04" ]]; then
    cp "$script_dir/ubuntu-2004-multi-config" "./$update_config_copy_path" || $ret 1
else
    echo "Unsupported distro: $distro"
    print_help
    exit 1
fi

$SUDO "./$update_build_command_path" || $ret 1

popd || $ret 1
#
# Now we should have both the image to flash and the update image
# time to copy over to the output directory
#
output_image=""
if [[ $distro == "ubuntu-18.04" ]]; then
    output_image="ubuntu-18.04.5-preinstalled-server-arm64+raspi.img"
elif [[ $distro == "ubuntu-20.04" ]]; then
    output_image="ubuntu-20.04.5-preinstalled-server-arm64+raspi.img"
else
    echo "Unsupported distro: $distro"
    print_help
    exit 1
fi

if [ ! -d "$out" ]; then
    mkdir -p "$out" || $ret 1
fi

$SUDO cp "$workspace/image_to_flash/$base_module_path/workspace/$output_image" "$out/image_to_flash.img" || $ret 1

$SUDO cp "$workspace/update_image/$update_module_path/workspace/$output_image" "$out/update_image.img" || $ret 1

# Now we need to create the update file
$SUDO gzip "$out/update_image.img" || $ret 1

swu_hash=$($SUDO openssl dgst -sha256 "$out/update_image.img.gz" | awk '{print $2}')

# Now we need to create the sw-description file
$SUDO $(cat "$script_dir/sw_descr_file" | sed "s/<<hash>>/${swu_hash}/" | sed "s/<<file>>/update_image.img.gz/") > "$out/sw-description" || $ret 1

$SUDO openssl dgst -sha256 -sign "$script_dir/keys/priv.pem" -out "$out/sw-description.sig" -passin "$script_dir/keys/priv.pass" "$out/sw-description"

echo -e "$out/sw-description\n$out/sw-description.sig\n$out/update_image.img.gz" > name-list

cpio -H crc -o < name-list > "$out/deviceupdate-update-file.swu"

popd || $ret 1

$SUDO rm -rf "$workspace" || $ret 1
