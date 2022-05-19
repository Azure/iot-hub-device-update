#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# install-deps.sh makes it more convenient to install
# dependencies for ADU Agent and Delivery Optimization.
# Some dependencies are installed via packages and
# others are installed from source code.

# Ensure that getopt starts from first option if ". <script.sh>" was used.
OPTIND=1

# Ensure we dont end the user's terminal session if invoked from source (".").
if [[ $0 != "${BASH_SOURCE[0]}" ]]; then
    ret=return
else
    ret=exit
fi

# Use sudo if user is not root
SUDO=""
if [ "$(id -u)" != "0" ]; then
    SUDO="sudo"
fi

warn() { echo -e "\033[1;33mWarning:\033[0m $*" >&2; }

error() { echo -e "\033[1;31mError:\033[0m $*" >&2; }

# Setup defaults
install_all_deps=false
install_packages=false
install_packages_only=false
# The folder where source code will be placed
# for building and installing from source.
work_folder=/tmp
keep_source_code=false
use_ssh=false

# ADUC Deps

install_aduc_deps=false
install_azure_iot_sdk=false
azure_sdk_ref=LTS_07_2021_Ref01

install_catch2=false
default_catch2_ref=v2.11.0
catch2_ref=$default_catch2_ref

install_azure_blob_storage_file_upload_utility=false
azure_blob_storage_file_upload_utility_ref=main

install_cmake_from_source=false
install_cmake_version=3.10.2

# DO Deps
default_do_ref=v0.8.2
install_do=false
do_ref=$default_do_ref

# Dependencies packages
aduc_packages=('git' 'make' 'build-essential' 'cmake' 'ninja-build' 'libcurl4-openssl-dev' 'libssl-dev' 'uuid-dev' 'python2.7' 'lsb-release' 'curl' 'wget' 'pkg-config')
static_analysis_packages=('clang' 'clang-tidy' 'cppcheck')
compiler_packages=("gcc-[68]")
do_packages=('libproxy-dev' 'libssl-dev' 'zlib1g-dev' 'libboost-all-dev')

# Distro info
OS=""
VER=""

print_help() {
    echo "Usage: install-deps.sh [options...]"
    echo "-a, --install-all-deps    Install all dependencies."
    echo "                          Implies --install-aduc-deps, --install-do, --install-do-deps, and --install-packages."
    echo "                          Can be used with --install-packages-only."
    echo "                          This is the default if no options are specified."
    echo ""
    echo "--install-aduc-deps       Install dependencies for ADU Agent."
    echo "                          Implies --install-azure-iot-sdk and --install-catch2."
    echo "                          When used with --install-packages will also install the package dependencies."
    echo "--install-azure-iot-sdk   Install the Azure IoT C SDK from source."
    echo "--azure-iot-sdk-ref <ref> Install the Azure IoT C SDK from a specific branch or tag."
    echo "                           Default is public-preview."
    echo "--install-abs-file-upload-utility   Install the Azure Blob Storage File Upload Utility from source."
    echo "--abs-file-upload-utility-ref <ref> Install the Azure Blob Storage File Upload Utility from a specific branch or tag."
    echo "--install-catch2          Install Catch2 from source."
    echo "--catch2-ref              Install Catch2 from a specific branch or tag."
    echo "                          This value is passed to git clone as the --branch argument."
    echo "                          Default is $default_catch2_ref."
    echo ""
    echo "--install-do              Install Delivery Optimization from source."
    echo "                          In order to install the correct dependencies, "
    echo "--do-ref <ref>            Install the DO source from this branch or tag."
    echo "                          This value is passed to git clone as the --branch argument."
    echo "                          Default is $default_do_ref."
    echo "--do-commit <commit_sha>  Specific commit to fetch."
    echo "                          Default is the latest commit in that branch."
    echo ""
    echo "-p, --install-packages    Indicates that packages should be installed."
    echo "--install-packages-only   Indicates that only packages should be installed and that dependencies should not be installed from source."
    echo ""
    echo "-f, --work-folder <work_folder>   Specifies the folder where source code will be cloned or downloaded."
    echo "                                  Default is /tmp."
    echo "-k, --keep-source-code            Indicates that source code should not be deleted after install from work_folder."
    echo ""
    echo "--use-ssh                 Use ssh URLs to clone instead of https URLs."
    echo "--list-deps               List the states of the dependencies."
    echo "-h, --help                Show this help message."
    echo ""
    echo "Example: ${BASH_SOURCE[0]} --install-all-deps --work-folder ~/adu-linux-client-deps --keep-source-code"
}

do_install_aduc_packages() {
    echo "Installing dependency packages for ADU Agent..."

    $SUDO apt-get install --yes "${aduc_packages[@]}" || return

    # The latest version of gcc available on Debian is gcc-6. We install that version if we are
    # building for Debian, otherwise we install gcc-8 for Ubuntu.
    OS=$(lsb_release --short --id)
    if [[ $OS == "debian" && $VER == "9" ]]; then
        $SUDO apt-get install --yes gcc-6 g++-6 || return
    else
        $SUDO apt-get install --yes gcc-8 g++-8 || return
    fi

    echo "Installing packages required for static analysis..."

    # The following is a workaround as IoT SDK references the following paths which don't exist
    # on our target platforms, and without these folders existing, static analysis will report:
    # (information) Couldn't find path given by -I '/usr/local/inc/'
    # (information) Couldn't find path given by -I '/usr/local/pal/linux/'
    $SUDO mkdir --parents /usr/local/inc /usr/local/pal/linux

    # Note that clang-tidy requires clang to be installed so that it can find clang headers.
    $SUDO apt-get install --yes "${static_analysis_packages[@]}" || return

    if [[ $install_cmake_from_source ]]; then
        do_install_cmake_from_source
    fi
}

do_install_azure_iot_sdk() {
    echo "Installing Azure IoT C SDK ..."
    local azure_sdk_dir=$work_folder/azure-iot-sdk-c
    if [[ $keep_source_code != "true" ]]; then
        $SUDO rm -rf $azure_sdk_dir || return
    elif [[ -d $azure_sdk_dir ]]; then
        warn "$azure_sdk_dir already exists! Skipping Azure IoT C SDK."
        return 0
    fi

    local azure_sdk_url
    if [[ $use_ssh == "true" ]]; then
        azure_sdk_url=git@github.com:Azure/azure-iot-sdk-c.git
    else
        azure_sdk_url=https://github.com/Azure/azure-iot-sdk-c.git
    fi

    echo -e "Building azure-iot-sdk-c ...\n\tBranch: $azure_sdk_ref\n\tFolder: $azure_sdk_dir"
    mkdir -p $azure_sdk_dir || return
    pushd $azure_sdk_dir > /dev/null
    git clone --branch $azure_sdk_ref $azure_sdk_url . || return
    git submodule update --init || return

    mkdir cmake || return
    pushd cmake > /dev/null

    # use_http is required for uHTTP support.
    local azureiotsdkc_cmake_options=(
        "-Duse_amqp:BOOL=OFF"
        "-Duse_http:BOOL=ON"
        "-Duse_mqtt:BOOL=ON"
        "-Dskip_samples:BOOL=ON"
        "-Dbuild_service_client:BOOL=OFF"
        "-Dbuild_provisioning_service_client:BOOL=OFF"
    )

    if [[ $keep_source_code == "true" ]]; then
        # If source is wanted, presumably samples and symbols are useful as well.
        azureiotsdkc_cmake_options+=("-DCMAKE_BUILD_TYPE:STRING=Debug")
    else
        azureiotsdkc_cmake_options+=("-Dskip_samples=ON")
    fi

    cmake "${azureiotsdkc_cmake_options[@]}" .. || return

    cmake --build . || return
    $SUDO cmake --build . --target install || return

    popd > /dev/null
    popd > /dev/null

    if [[ $keep_source_code != "true" ]]; then
        $SUDO rm -rf $azure_sdk_dir || return
    fi
}

do_install_catch2() {
    echo "Installing Catch2 ..."
    local catch2_dir=$work_folder/catch2

    if [[ $keep_source_code != "true" ]]; then
        $SUDO rm -rf $catch2_dir || return
    elif [[ -d $catch2_dir ]]; then
        warn "$catch2_dir already exists! Skipping Catch2."
        return 0
    fi

    local catch2_url
    if [[ $use_ssh == "true" ]]; then
        catch2_url=git@github.com:catchorg/Catch2.git
    else
        catch2_url=https://github.com/catchorg/Catch2.git
    fi

    echo -e "Building Catch2 ...\n\tBranch: $catch2_ref\n\tFolder: $catch2_dir"
    mkdir -p $catch2_dir || return
    pushd $catch2_dir > /dev/null
    git clone --recursive --single-branch --branch $catch2_ref --depth 1 $catch2_url . || return

    mkdir cmake || return
    pushd cmake > /dev/null
    cmake .. || return
    cmake --build . || return
    $SUDO cmake --build . --target install || return
    popd > /dev/null
    popd > /dev/null

    if [[ $keep_source_code != "true" ]]; then
        $SUDO rm -rf $catch2_dir
    fi
}

do_install_do() {
    echo "Installing DO ..."
    local do_dir=$work_folder/do

    if [[ $keep_source_code != "true" ]]; then
        $SUDO rm -rf $do_dir || return
    elif [[ -d $do_dir ]]; then
        warn "$do_dir already exists! Skipping DO."
        return 0
    fi

    echo -e "Building DO ...\n\tBranch: $do_ref\n\tFolder: $do_dir"
    mkdir -p $do_dir || return
    pushd $do_dir > /dev/null

    local do_url
    if [[ $use_ssh == "true" ]]; then
        do_url=git@github.com:Microsoft/do-client.git
    else
        do_url=https://github.com/Microsoft/do-client.git
    fi

    git clone --recursive --single-branch --branch $do_ref --depth 1 $do_url . || return

    distro=$OS$VER
    install_do_deps_distro="${distro//.}"

    if [[ $install_do_deps_distro != "" ]]; then
        local bootstrap_file=$do_dir/build/scripts/bootstrap.sh
        chmod +x $bootstrap_file || return
        $SUDO $bootstrap_file --platform "$install_do_deps_distro" --install build || return
    fi

    mkdir cmake || return
    pushd cmake > /dev/null

    local do_cmake_options=(
        "-DDO_BUILD_TESTS:BOOL=OFF"
        "-DDO_INCLUDE_SDK=ON"
    )

    if [[ $keep_source_code == "true" ]]; then
        do_cmake_options+=("-DCMAKE_BUILD_TYPE=Debug")
    else
        do_cmake_options+=("-DCMAKE_BUILD_TYPE=Release")
    fi

    cmake "${do_cmake_options[@]}" .. || return
    cmake --build . || return
    $SUDO cmake --build . --target install || return
    popd > /dev/null
    popd > /dev/null

    if [[ $keep_source_code != "true" ]]; then
        $SUDO rm -rf $do_dir
    fi
}

do_install_azure_blob_storage_file_upload_utility() {
    echo "Installing azure-blob-storage-file-upload-utility from source."
    local abs_fuu_dir=$work_folder/azure_blob_storage_file_upload_utility

    if [[ $keep_source_code != "true" ]]; then
        $SUDO rm -rf $abs_fuu_dir || return
    elif [[ -d $abs_fuu_dir ]]; then
        warn "$abs_fuu_dir already exists! Skipping Azure Storage CPP Lite."
        return 0
    fi

    local azure_storage_cpplite_url
    if [[ $use_ssh == "true" ]]; then
        azure_storage_cpplite_url=git@github.com:Azure/azure-blob-storage-file-upload-utility.git
    else
        azure_storage_cpplite_url=https://github.com/Azure/azure-blob-storage-file-upload-utility.git
    fi

    echo -e "Cloning Azure Blob Storage File Upload Uility ...\n\tBranch: $azure_blob_storage_file_upload_utility_ref\n\t Folder: $abs_fuu_dir"
    mkdir -p $abs_fuu_dir || return
    pushd $abs_fuu_dir > /dev/null
    git clone --recursive --single-branch --branch $azure_blob_storage_file_upload_utility_ref --depth 1 $azure_storage_cpplite_url . || return

    echo -e "Installing Azure Blob Storage File Upload Utiltiy dependencies..."

    # Note added to make sure that install-deps.sh is executable
    chmod u+x ./scripts/install-deps.sh

    # Note we can skip the azure iot sdk installation because it is guaranteed that it will already be installed.
    ./scripts/install-deps.sh -a --skip-azure-iot-sdk-install

    mkdir cmake || return
    pushd cmake > /dev/null

    local azure_blob_storage_file_upload_utility_cmake_options
    if [[ $keep_source_code == "true" ]]; then
        # If source is wanted, presumably samples and symbols are useful as well.
        azure_blob_storage_file_upload_utility_cmake_options+=("-DCMAKE_BUILD_TYPE:STRING=Debug")
    else
        azure_blob_storage_file_upload_utility_cmake_options+=("-DCMAKE_BUILD_TYPE:STRING=Release")
    fi

    echo -e "Building Azure Blob Storage File Upload Uility ...\n\tBranch: $azure_blob_storage_file_upload_utility_ref\n\t"
    cmake "${azure_blob_storage_file_upload_utility_cmake_options[@]}" .. || return

    cmake --build . || return
    $SUDO cmake --build . --target install || return

    popd > /dev/null
    popd > /dev/null

    if [[ $keep_source_code != "true" ]]; then
        $SUDO rm -rf $abs_fuu_dir || return
    fi
}

do_install_cmake_from_source() {
    local cmake_url
    local cmake_tar_path
    local cmake_dir_path

    if [[ $install_cmake_version == "3.10.2" ]]; then
        cmake_url="https://cmake.org/files/v3.10/cmake-3.10.2.tar.gz"
        cmake_tar_path="$work_folder/cmake-3.10.2.tar.gz"
        cmake_dir_path="$work_folder/cmake-3.10.2/"

        wget -P $work_folder ${cmake_url}
        tar -zxvf ${cmake_tar_path} -C ${work_folder}

        pushd $cmake_dir_path > /dev/null

        ./bootstrap
        make > /dev/null
        popd > /dev/null
    elif [[ $install_cmake_version == "3.19.8" ]]; then
        # Later it will be added when the cmake linking error is solved
        # Task 38390989: Move to latest CMake at least 3.19+ to get Ubuntu 20.04+ working
        error "Currently CMake 3.13+ is not supported. It will be added in June 2022. "
    else
        echo "Not a supported cmake version"
    fi
}

determine_distro() {
    # shellcheck disable=SC1091

    # Checking distro name and version
    if [ -r /etc/os-release ]; then
        # freedesktop.org and systemd
        OS=$(grep "^ID\s*=\s*" /etc/os-release | sed -e "s/^ID\s*=\s*//")
        VER=$(grep "^VERSION_ID=" /etc/os-release | sed -e "s/^VERSION_ID=//")
        VER=$(sed -e 's/^"//' -e 's/"$//' <<<"$VER")
    elif type lsb_release >/dev/null 2>&1; then
        # linuxbase.org
        OS=$(lsb_release -si)
        VER=$(lsb_release -sr)
    elif [ -f /etc/lsb-release ]; then
        # For some versions of Debian/Ubuntu without lsb_release command
        . /etc/lsb-release
        OS=$DISTRIB_ID
        VER=$DISTRIB_RELEASE
    elif [ -f /etc/debian_version ]; then
        # Older Debian/Ubuntu/etc.
        OS=Debian
        VER=$(cat /etc/debian_version)
    else
        # Fall back to uname, e.g. "Linux <version>", also works for BSD, etc.
        OS=$(uname -s)
        VER=$(uname -r)
    fi
    # Covert OS to lowercase 
    OS="$(echo "$OS" | tr '[:upper:]' '[:lower:]')"
}

do_list_all_deps() {
    declare -a deps_set=()
    deps_set+=(${aduc_packages[@]})
    deps_set+=(${compiler_packages[@]})
    deps_set+=(${static_analysis_packages[@]})
    deps_set+=(${do_packages[@]})
    echo "Listing the state of dependencies:"
    dpkg-query -W -f='${binary:Package} ${Version} (${Architecture})\n' "${deps_set[@]}"
    ret_val=$?
    if [ $ret_val -eq 1 ]; then
        warn "dpkg-query failed"
        return 0
    elif [ $ret_val -ge 2 ]; then
        error "dpkg-query failed with status $ret_val"
        return $ret_val
    fi
    return 0
}

###############################################################################

# Check if no options were specified.
if [[ $1 == "" ]]; then
    error "Must specify at least one option."
    $ret 1
fi

determine_distro

if [[ $OS != "ubuntu" || $VER != "18.04" ]]; then
    install_cmake_from_source=true
fi

# Parse cmd options
while [[ $1 != "" ]]; do
    case $1 in
    -a | --install-all-deps)
        install_all_deps=true
        ;;
    --install-aduc-deps)
        install_aduc_deps=true
        ;;
    --install-azure-iot-sdk)
        install_azure_iot_sdk=true
        ;;
    --azure-iot-sdk-ref)
        shift
        azure_sdk_ref=$1
        ;;
    --install-abs-file-upload-utility)
        shift
        install_azure_blob_storage_file_upload_utility=true
        ;;
    --abs-file-upload-utility-ref)
        shift
        azure_blob_storage_file_upload_utility_ref=$1
        ;;
    --install-catch2)
        install_catch2=true
        ;;
    --catch2-ref)
        shift
        catch2_ref=$1
        ;;
    --install-do)
        install_do=true
        ;;
    --do-ref)
        shift
        do_ref=$1
        ;;
    -p | --install-packages)
        install_packages=true
        ;;
    --install-packages-only)
        install_packages_only=true
        ;;
    -f | --work-folder)
        shift
        work_folder=$(realpath "$1")
        ;;
    -k | --keep-source-code)
        keep_source_code=true
        ;;
    --use-ssh)
        use_ssh=true
        ;;
    --list-deps)
        do_list_all_deps
        $ret $?
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

# If there is no install action specified,
# assume that we want to install all deps.
if [[ $install_all_deps != "true" && $install_aduc_deps != "true" && $install_do != "true" && $install_azure_iot_sdk != "true" && $install_catch2 != "true" ]]; then
    install_all_deps=true
fi

# If --all was specified,
# set all install actions to "true".
if [[ $install_all_deps == "true" ]]; then
    install_aduc_deps=true
    install_do=true
    install_packages=true
fi

# Set implied options for aduc deps.
if [[ $install_aduc_deps == "true" ]]; then
    install_azure_iot_sdk=true
    install_catch2=true
    install_azure_blob_storage_file_upload_utility=true
fi

# Set implied options for packages only.
if [[ $install_packages_only == "true" ]]; then
    install_packages=true
    install_azure_iot_sdk=false
    install_catch2=false
fi

if [[ $install_packages == "true" ]]; then
    # Check if we need to install any packages
    # before we call apt update.
    if [[ $install_aduc_deps == "true" ]]; then
        echo "Updating repository list..."
        $SUDO apt-get update --yes --fix-missing --quiet || $ret
    fi
fi

if [[ $install_aduc_deps == "true" ]]; then
    do_install_aduc_packages || $ret
fi

# Install dependencies from source
if [[ $install_packages_only == "false" ]]; then
    if [[ $install_azure_iot_sdk == "true" ]]; then
        do_install_azure_iot_sdk || $ret
    fi

    if [[ $install_catch2 == "true" ]]; then
        do_install_catch2 || $ret
    fi

    if [[ $install_do == "true" ]]; then
        do_install_do || $ret
    fi

    if [[ $install_azure_blob_storage_file_upload_utility == "true" ]]; then
        do_install_azure_blob_storage_file_upload_utility || $ret
    fi
fi

# After installation, it prints out the states of dependencies
if [[ $install_aduc_deps == "true" || $install_do == "true" || $install_packages_only == "true" || $install_packages == "true" ]]; then
    do_list_all_deps || $ret $?
fi
