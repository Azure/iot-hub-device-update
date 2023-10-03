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

warn() { echo -e "\033[1;33mWarning:\033[0m $*" >&2; }

error() { echo -e "\033[1;31mError:\033[0m $*" >&2; }

openssl_dir_path="/usr/local/lib/deviceupdate-openssl"
openssl_version="3.1.2"
DEFAULT_WORKFOLDER=/tmp
work_folder=$DEFAULT_WORKFOLDER

print_help() {
    echo "Usage: install-openssl3.sh [options...]"
    echo ""
    echo "--openssl-dir-path        Specific path to install OpenSSL 1.1"
    echo ""
    echo "--openssl-version         Specific version of OpenSSL to install. Only supports installations of 1.1x and 3.x"
    echo "                          Suggested versions are: 1.1.1w, 3.0.11, or 3.1.3"
    echo "                          If you absolutely want to live on the edge you can use this script to install 3.2.0-alpha1"
    echo "                          but results are unpredictable"
    echo " DO NOT USE THIS SCRIPT IF YOU DO NOT KNOW IF THE OPENSSL VERSION YOU ARE INSTALLING IS COMPATIBLE WITH YOUR CURRENT SYSTEM"
    echo " *********************** THIS CAN BRICK YOUR SSH CONNECTION TO A REMOTE MACHINE ***********************"
    echo "-h, --help                Show this help message."
    echo ""
}

do_install_openssl() {
    local openssl_dir="$work_folder/openss$openssl_version"

    if [[ -d $openssl_dir ]]; then
        $SUDO rm -rf "$openssl_dir" || return
    fi

    echo "Installing OpenSSL v$openssl_version..."

    $SUDO apt-get install -y build-essential checkinstall zlib1g-dev -y || return
    wget https://www.openssl.org/source/openssl-${openssl_version}.tar.gz || return
    mkdir -p "$openssl_dir" || return
    tar -xf openssl-${openssl_version}.tar.gz -C "$openssl_dir" --strip-components=1 || return
    pushd "$openssl_dir" > /dev/null || return
    ./config --prefix="$openssl_dir_path" --openssldir="$openssl_dir_path" shared zlib || return
    make || return

    $SUDO make install || return
    export LD_LIBRARY_PATH="$openssl_dir_path/lib:$openssl_dir_path/lib64:$LD_LIBRARY_PATH" || return
    echo "OpenSSL has been installed in $openssl_dir_path"

    popd > /dev/null || return

    local openssl_major_version=""
    openssl_major_version=$(echo $openssl_version | cut -d'.' -f1)
    local openssl_minor_version=""
    openssl_minor_version=$(echo $openssl_version | cut -d'.' -f2)

    local openssl_so_suffix=""
    if [[ $openssl_major_version -eq "1" && $openssl_minor_version -eq "1" ]]; then
        # Need to handle sym linking 1.1
        openssl_so_suffix="1.1"
    elif [[ $openssl_major_version -eq "3" ]]; then
        # Need to handle sym linking 3.x
        openssl_so_suffix="3"
    else
        error "OpenSSL version $openssl_version is not supported"
        error "Only installations of 1.1 and 3.x are supported"
        $ret 1
    fi

    if [[ ! -d "$openssl_dir_path/lib" ]]; then
        # Handle missing /lib/ directory
        $SUDO mkdir $openssl_dir_path/lib || $ret 1
        $SUDO ln -sf "$openssl_dir_path/lib64/libcrypto.so.$openssl_so_suffix" "$openssl_dir_path/lib/libcrypto.so" || return
        $SUDO ln -sf "$openssl_dir_path/lib64/libcrypto.so.$openssl_so_suffix" "$openssl_dir_path/lib/libcrypto.so.$openssl_so_suffix" || return

        # Create the symlinks for libssl
        $SUDO ln -sf "$openssl_dir_path/lib64/libssl.so.$openssl_so_suffix" "$openssl_dir_path/lib/libssl.so" || return
        $SUDO ln -sf "$openssl_dir_path/lib64/libssl.so.$openssl_so_suffix" "$openssl_dir_path/lib/libssl.so.$openssl_so_suffix" || return
    fi

    if [[ ! -d "$openssl_dir_path/lib64" ]]; then
        # Handle missing /lib64/ directory
        $SUDO mkdir $openssl_dir_path/lib64 || $ret 1

        $SUDO mkdir $openssl_dir_path/lib || $ret 1
        $SUDO ln -sf "$openssl_dir_path/lib/libcrypto.so.$openssl_so_suffix" "$openssl_dir_path/lib64/libcrypto.so" || return
        $SUDO ln -sf "$openssl_dir_path/lib/libcrypto.so.$openssl_so_suffix" "$openssl_dir_path/lib64/libcrypto.so.$openssl_so_suffix" || return

        # Create the symlinks for libssl
        $SUDO ln -sf "$openssl_dir_path/lib/libssl.so.$openssl_so_suffix" "$openssl_dir_path/lib64/libssl.so" || return
        $SUDO ln -sf "$openssl_dir_path/lib/libssl.so.$openssl_so_suffix" "$openssl_dir_path/lib64/libssl.so.$openssl_so_suffix" || return
    fi

    # Create the symlinks for runtime agents to find the libraries
    $SUDO ln -sf "$openssl_dir_path/lib64/libcrypto.so.$openssl_so_suffix" "/usr/lib/libcrypto.so.$openssl_so_suffix" || return
    $SUDO ln -sf "$openssl_dir_path/lib64/libssl.so.$openssl_so_suffix" "/usr/lib/libssl.so.$openssl_so_suffix" || return

    echo "Adding OpenSSL to ldconfig's configuration"
    $SUDO ldconfig $openssl_dir_path/lib64/ || return
    $SUDO ldconfig $openssl_dir_path/lib/ || return

    $SUDO rm -r "$openssl_dir" || return
    $SUDO rm "openssl-${openssl_version}.tar.gz" || return
}

###############################################################################

# Check if no options were specified.
if [[ $1 == "" ]]; then
    error "Must specify at least one option."
    $ret 1
fi

# Parse cmd options
while [[ $1 != "" ]]; do
    case $1 in
    --openssl-dir-path)
        shift
        openssl_dir_path=$1
        ;;
    --openssl-version)
        shift
        openssl_version=$1
        ;;
    -h | --help)
        print_help
        $ret 0
        ;;
    *)
        error "Invalid argument: $*"
        $ret 1
        ;;
    esac
    shift
done

do_install_openssl || $ret 1

$ret 0
