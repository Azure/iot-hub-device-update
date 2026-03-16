#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# install-deps.sh makes it more convenient to install
# dependencies for ADU Agent and Delivery Optimization.
# Some dependencies are installed via packages and
# others are installed from source code.

# Ensure that getopt starts from first option if ". <script.sh>" was used.
OPTIND=1

ret=""
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

# Determine the git root directory
GITROOT="$(git rev-parse --show-toplevel 2> /dev/null)"
if [ -z "$GITROOT" ]; then
    # If not in a git repo, use the script's parent directory
    GITROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
fi

# Setup defaults
install_all_deps=false
install_packages=false
install_packages_only=false
# The folder where source code will be placed
# for building and installing from source.
# Use parent directory of git root to avoid vcpkg manifest conflicts
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" > /dev/null 2>&1 && pwd)"
repo_root="$(cd "$script_dir/.." > /dev/null 2>&1 && pwd)"
DEFAULT_WORKFOLDER="$repo_root/.workspace"
work_folder=$DEFAULT_WORKFOLDER
keep_source_code=false
use_ssh=false

# ADUC Deps

install_aduc_deps=false
install_azure_iot_sdk=false
azure_sdk_ref=LTS_08_2023

# ADUC Diagnostics Deps
azure_storage_sdk_branch_ref=main
azure_storage_sdk_tag_ref=azure-core_1.6.0
install_azure_storage_sdk=false
# ADUC Test Deps

install_catch2=false
default_catch2_ref=v3.8.0
catch2_ref=$default_catch2_ref
install_swupdate=false
default_swupdate_ref=2021.11
swupdate_ref=$default_swupdate_ref

install_cmake=false
supported_cmake_version='3.23.2'
install_cmake_version="$supported_cmake_version"
cmake_force_source=false
cmake_prefix="$work_folder"
cmake_installer_dir=""
cmake_bin="cmake"

install_shellcheck=false
supported_shellcheck_version='0.8.0'

install_valgrind=false
valgrind_install_method="apt" # apt or source
supported_valgrind_version='3.23.0'
valgrind_ref="VALGRIND_3_23_0"

install_githooks=false

du_test_data_dir_path="/tmp/adu/"
# DO Deps
default_do_ref=develop
install_do=false
do_ref=$default_do_ref

# Default delta ref uses GCC 12+ compatible branch
default_delta_ref=feature/vnext-delta
install_delta=false
delta_ref=$default_delta_ref

# CMake symlink location
cmake_dir_symlink="$repo_root/.workspace/deviceupdate-cmake"

# catch2 build
#
# used for dependencies like catch2 that will find system default
# (e.g. g++-9) despite g++-10 installed, so use CC and CXX env
# vars that CMake will honor.
catch2_cc=""
catch2_cxx=""

# Check if a dependency is already installed at the expected version.
# Usage: is_dep_installed <name> <version>
# Returns 0 (true) if the stamp file exists and matches the version.
is_dep_installed() {
    local name="$1" version="$2"
    local stamp="$deps_stamp_dir/$name"
    if [[ -f $stamp ]] && [[ "$(cat "$stamp" 2> /dev/null)" == "$version" ]]; then
        return 0
    fi
    return 1
}

# Record that a dependency was successfully installed.
# Usage: mark_dep_installed <name> <version>
mark_dep_installed() {
    local name="$1" version="$2"
    mkdir -p "$deps_stamp_dir"
    echo "$version" > "$deps_stamp_dir/$name"
}

# Dependencies packages
aduc_packages=('git' 'make' 'build-essential' 'cmake' 'ninja-build' 'libcurl4-openssl-dev' 'libssl-dev' 'uuid-dev' 'lsb-release' 'curl' 'wget' 'pkg-config' 'libxml2-dev' 'file')
static_analysis_packages=('clang' 'clang-tidy' 'cppcheck')
compiler_packages=('gcc' 'g++')

# Distro and arch info
OS=""
VER=""
is_amd64=false
is_arm64=false
is_arm32=false

print_help() {
    echo "Usage: install-deps.sh [options...]"
    echo "-a, --install-all-deps    Install all dependencies."
    echo "                          Implies --install-aduc-deps, --install-do, --install-packages, --install-cmake, and --install-shellcheck."
    echo "                          Can be used with --install-packages-only."
    echo "                          This is the default if no options are specified."
    echo ""
    echo "--install-aduc-deps       Install dependencies for ADU Agent."
    echo "                          Implies --install-azure-iot-sdk and --install-catch2."
    echo "                          When used with --install-packages will also install the package dependencies."
    echo "--install-azure-iot-sdk   Install the Azure IoT C SDK from source."
    echo "--install-azure-storage-sdk Install the Azure SDK for CPP from source."
    echo "--azure-iot-sdk-ref <ref> Install the Azure IoT C SDK from a specific branch or tag."
    echo "                           Default is public-preview."
    echo "--install-catch2          Install Catch2 from source."
    echo "--install-cmake           Installs supported version of cmake from installer if on ubuntu, else installs it from source."
    echo "--install-shellcheck      Installs supported version of shellcheck."
    echo "--install-valgrind [method] Install Valgrind for memory leak detection."
    echo "                          method can be: apt or source."
    echo "                          'apt' installs from package manager."
    echo "                          'source' builds from source (version $supported_valgrind_version)."
    echo "--cmake-prefix            Set the install path prefix when --install-cmake is used. Default is [git-root]/.tmp."
    echo "--cmake-version           Override the version of CMake. e.g. 3.23.2 that will be installed if --install-cmake is used."
    echo "--cmake-force-source      Force building cmake from source when --install-cmake is used."
    echo "--install-githooks        Install githooks required by the repository."
    echo "--catch2-ref              Install Catch2 from a specific branch or tag."
    echo "                          This value is passed to git clone as the --branch argument."
    echo "                          Default is $default_catch2_ref."
    echo ""
    echo "--install-swupdate        Build and install the SWUpdate project. (required for SWUpdate unit tests on Ubuntu)"
    echo "--swupdate-ref            <ref> Clone the SWUpdate project from a specific branch or tag."
    echo "                           Default is $default_swupdate_ref."
    echo ""
    echo "--install-do              Install Delivery Optimization from source."
    echo "                          In order to install the correct dependencies, "
    echo "--do-ref <ref>            Install the DO source from this branch or tag."
    echo "                          This value is passed to git clone as the --branch argument."
    echo "                          Default is $default_do_ref."
    echo "--do-commit <commit_sha>  Specific commit to fetch."
    echo "                          Default is the latest commit in that branch."
    echo ""
    echo "--install-delta           Install iot-hub-device-update-delta library from source."
    echo "--delta-ref <ref>         Install the delta library from this branch or tag."
    echo "                          This value is passed to git clone as the --branch argument."
    echo "                          Default is $default_delta_ref."
    echo ""
    echo "-p, --install-packages    Indicates that packages should be installed."
    echo "--install-packages-only   Indicates that only packages should be installed and that dependencies should not be installed from source."
    echo ""
    echo "-f, --work-folder <work_folder>   Specifies the folder where temp artifacts will be stored."
    echo "                                  This folder contains temporary build artifacts for dependencies,"
    echo "                                  CMake/shellcheck installations, and test data."
    echo "                                  Default is [git-root]/.tmp."
    echo "-k, --keep-source-code            Indicates that source code should not be deleted after install from work_folder."
    echo ""
    echo "--use-ssh                 Use ssh URLs to clone instead of https URLs."
    echo ""
    echo "--list-deps               List the states of the dependencies."
    echo "-h, --help                Show this help message."
    echo ""
    echo "Example: ${BASH_SOURCE[0]} --install-all-deps --work-folder ~/adu-linux-client-deps --keep-source-code"
}

do_install_valgrind_from_apt() {
    echo "Installing Valgrind from apt..."
    $SUDO apt-get install --yes valgrind || return
    echo "Valgrind installed from apt successfully."
}

do_install_valgrind_from_source() {
    echo "Installing Valgrind from source..."
    local valgrind_dir=$work_folder/valgrind
    if [[ -d $valgrind_dir ]]; then
        $SUDO rm -rf "$valgrind_dir" || return
    fi

    local valgrind_url
    if [[ $use_ssh == "true" ]]; then
        valgrind_url=git@github.com:valgrind/valgrind.git
    else
        valgrind_url=https://github.com/valgrind/valgrind.git
    fi

    echo -e "Building Valgrind from source...\n\tTag: $valgrind_ref\n\tFolder: $valgrind_dir"
    mkdir -p "$valgrind_dir" || return
    pushd "$valgrind_dir" > /dev/null || return
    git clone --branch "$valgrind_ref" --depth 1 "$valgrind_url" . || return

    ./autogen.sh || return
    ./configure --prefix=/usr/local || return
    make -j"$(nproc)" || return
    $SUDO make install || return

    popd > /dev/null || return

    if [[ $keep_source_code != "true" ]]; then
        echo "Removing Valgrind source code..."
        $SUDO rm -rf "$valgrind_dir"
    fi

    # Create symlink if not already exists
    if [[ ! -L /usr/bin/valgrind ]]; then
        $SUDO ln -sf /usr/local/bin/valgrind /usr/bin/valgrind || return
    fi

    echo "Valgrind installed from source successfully."
}

do_install_githooks() {
    echo "Installing githooks..."

    GITROOT="$(git rev-parse --show-toplevel 2> /dev/null)"

    if [ -z "$GITROOT" ]; then
        echo 'Unable to determine git root.' >&2
        $ret 1
    fi

    if ! $SUDO ln -sf "$GITROOT/scripts/githooks/pre-commit.sh" "$GITROOT/.git/hooks/pre-commit"; then
        echo "Unable to symlink pre-commit. exit code: $?"
        $ret 1
    fi
}

do_install_aduc_packages() {
    echo "Installing dependency packages for ADU Agent..."

    $SUDO apt-get install --yes "${aduc_packages[@]}" || return

    # For Ubuntu 24.04+, ensure the 'file' utility is installed (may be needed by CPack)
    OS=$(lsb_release --short --id)
    if [[ $OS == "Ubuntu" ]]; then
        # Parse version to check if 24.04 or later
        VER_MAJOR=$(echo "$VER" | cut -d. -f1)
        VER_MINOR=$(echo "$VER" | cut -d. -f2)
        if [[ $VER_MAJOR -gt 24 ]] || [[ $VER_MAJOR -eq 24 && $VER_MINOR -ge 4 ]]; then
            echo "Ensuring 'file' utility is available for Ubuntu 24.04+"
            $SUDO apt-get install --yes file || echo "Warning: Could not install 'file' package"
        fi
    fi

    # The latest version of gcc available on Debian is gcc-6. We install that version if we are
    # building for Debian, otherwise we install gcc-8 for Ubuntu.
    if [[ $OS == "Debian" && $VER == "9" ]]; then
        $SUDO apt-get install --yes gcc-6 g++-6 || return
        catch2_cc=/usr/bin/gcc-6
        catch2_cxx=/usr/bin/g++-6
    elif [[ ($OS == "Debian" && $VER == "11") || (\
        $OS == "Ubuntu" && $VER == "20.04") || (\
        $OS == "Ubuntu" && $VER == "22.04") ]] \
            ; then
        $SUDO apt-get install --yes gcc-10 g++-10 || return
        catch2_cc=/usr/bin/gcc-10
        catch2_cxx=/usr/bin/g++-10
    elif [[ $OS == "Debian" && $VER == "12" ]]; then
        $SUDO apt-get install --yes gcc-12 g++-12 || return
        catch2_cc=/usr/bin/gcc-12
        catch2_cxx=/usr/bin/g++-12
    elif [[ $OS == "Debian" && $VER == "13" ]]; then
        # Debian 13 (trixie) - use gcc-12 for consistency with Debian 12
        $SUDO apt-get install --yes gcc-12 g++-12 || return
        catch2_cc=/usr/bin/gcc-12
        catch2_cxx=/usr/bin/g++-12
    elif [[ $OS == "Ubuntu" && $VER == "24.04" ]]; then
        # Ubuntu 24.04 and newer have a recent enough default gcc, so we don't need to install a specific version
        echo "Using system default gcc for Ubuntu 24.04+"
        catch2_cc=/usr/bin/gcc
        catch2_cxx=/usr/bin/g++
    else
        $SUDO apt-get install --yes gcc-8 g++-8 || return
        catch2_cc=/usr/bin/gcc-8
        catch2_cxx=/usr/bin/g++-8
    fi

    echo "Installing packages required for static analysis..."

    # The following is a workaround as IoT SDK references the following paths which don't exist
    # on our target platforms, and without these folders existing, static analysis will report:
    # (information) Couldn't find path given by -I '/usr/local/inc/'
    # (information) Couldn't find path given by -I '/usr/local/pal/linux/'
    $SUDO mkdir --parents /usr/local/inc /usr/local/pal/linux

    # Note that clang-tidy requires clang to be installed so that it can find clang headers.
    $SUDO apt-get install --yes "${static_analysis_packages[@]}" || return
}

do_install_azure_iot_sdk() {
    echo "Installing Azure IoT C SDK ..."

    if is_dep_installed "azure-iot-sdk-c" "$azure_sdk_ref"; then
        echo "Azure IoT C SDK ($azure_sdk_ref) already installed. Skipping..."
        return 0
    fi

    local azure_sdk_dir=$work_folder/azure-iot-sdk-c
    if [[ -d $azure_sdk_dir ]]; then
        $SUDO rm -rf "$azure_sdk_dir" || return
    fi

    local azure_sdk_url
    if [[ $use_ssh == "true" ]]; then
        azure_sdk_url=git@github.com:Azure/azure-iot-sdk-c.git
    else
        azure_sdk_url=https://github.com/Azure/azure-iot-sdk-c.git
    fi

    echo -e "Building azure-iot-sdk-c ...\n\tBranch: $azure_sdk_ref\n\tFolder: $azure_sdk_dir"
    mkdir -p "$azure_sdk_dir" || return
    pushd "$azure_sdk_dir" > /dev/null || return
    git clone --branch "$azure_sdk_ref" "$azure_sdk_url" . || return
    git submodule update --init || return

    mkdir cmake || return
    pushd cmake > /dev/null || return

    # use_http is required for uHTTP support.
    local azureiotsdkc_cmake_options=(
        "-Duse_amqp:BOOL=OFF"
        "-Duse_http:BOOL=ON"
        "-Duse_mqtt:BOOL=ON"
        "-Duse_wsio:BOOL=ON"
        "-Dskip_samples:BOOL=ON"
        "-Dbuild_service_client:BOOL=OFF"
        "-Dbuild_provisioning_service_client:BOOL=OFF"
        "-Duse_prov_client:BOOL=OFF"
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

    popd > /dev/null || return
    popd > /dev/null || return

    mark_dep_installed "azure-iot-sdk-c" "$azure_sdk_ref"

    if [[ $keep_source_code != "true" ]]; then
        $SUDO rm -rf "$azure_sdk_dir" || return
    fi
}

do_install_catch2() {
    echo "Installing Catch2 ..."

    if is_dep_installed "catch2" "$catch2_ref"; then
        echo "Catch2 ($catch2_ref) already installed. Skipping..."
        return 0
    fi

    local catch2_dir=$work_folder/catch2
    if [[ -d $catch2_dir ]]; then
        $SUDO rm -rf "$catch2_dir" || return
    fi

    local catch2_url
    if [[ $use_ssh == "true" ]]; then
        catch2_url=git@github.com:catchorg/Catch2.git
    else
        catch2_url=https://github.com/catchorg/Catch2.git
    fi

    echo -e "Building Catch2 ...\n\tBranch: $catch2_ref\n\tFolder: $catch2_dir"
    mkdir -p "$catch2_dir" || return
    pushd "$catch2_dir" > /dev/null || return
    git clone --recursive --single-branch --branch "$catch2_ref" --depth 1 "$catch2_url" . || return

    mkdir cmake || return
    pushd cmake > /dev/null || return

    CC="$catch2_cc" CXX="$catch2_cxx" "$cmake_bin" .. || return
    CC="$catch2_cc" CXX="$catch2_cxx" "$cmake_bin" --build . || return
    $SUDO "$cmake_bin" --build . --target install || return
    popd > /dev/null || return
    popd > /dev/null || return

    mark_dep_installed "catch2" "$catch2_ref"

    if [[ $keep_source_code != "true" ]]; then
        $SUDO rm -rf "$catch2_dir" || return
    fi
}

do_install_swupdate() {
    echo "Installing SWupdate ($swupdate_ref) ..."

    # Currently only support building SWUpdate on following distros:
    #   - Ubuntu 18.04
    #   - Ubuntu 20.04
    lsb_release -a | grep -e 'Ubuntu 18.04' -e 'Ubuntu 20.04'
    local grep_res=$?
    if [[ $grep_res -ne "0" ]]; then
        echo "Only need to build SWUpdate for Ubuntu 18.04 and Ubuntu 20.04. Skipping..."
        return 0
    fi

    local swupdate_dir=$work_folder/swupdate
    if [[ -d $swupdate_dir ]]; then
        $SUDO rm -rf "$swupdate_dir" || return 1
    fi

    local swupdate_url
    if [[ $use_ssh == "true" ]]; then
        swupdate_url=git@github.com:sbabic/swupdate.git
    else
        swupdate_url=https://github.com/sbabic/swupdate.git
    fi

    echo -e "Building SWUpdate ...\n\tBranch: $swupdate_ref\n\tFolder: $swupdate_dir"
    mkdir -p "$swupdate_dir" || return
    pushd "$swupdate_dir" > /dev/null || return
    git clone --recursive --single-branch --branch "$swupdate_ref" --depth 1 "$swupdate_url" . || return

    popd > /dev/null || return
    echo -e "Customizing SWUpdate build configurations..."
    cp src/deps/swupdate/.config "$swupdate_dir" || return
    pushd "$swupdate_dir" > /dev/null || return

    echo -r "Building SWUpdate..."
    make || return

    echo -e "Installing SWUpdate..."
    $SUDO make install || return
    popd > /dev/null || return

    if [[ $keep_source_code != "true" ]]; then
        $SUDO rm -rf "$swupdate_dir" || return 1
    fi
}

do_install_do_release_tarball() {
    local ret=0
    local dist=''
    local arch=''
    local do_release_tarball_url=''
    local tarball_filename=''
    local do_dir="$work_folder/do"

    echo -e "Attempting to install libdeliveryoptimization from release tarball...\n"

    local os_lowercase="${OS,,}"

    echo "os_lowercase => $os_lowercase"
    echo "VER => $VER"
    echo "is_arm32 => $is_arm32"
    echo "is_arm64 => $is_arm64"
    echo "is_amd64 => $is_amd64"

    if [[ $os_lowercase == "debian" && $VER == "9" ]]; then
        echo "evaluating debian9 for supported DO tarball ..."
        dist='debian9'
        if [[ $is_arm32 == "true" ]]; then
            arch='arm32'
        else
            warn "unsupported arch for DO release asset on Debian9. Supported: arm32"
            return 1
        fi
    elif [[ $os_lowercase == "debian" && $VER == "10" ]]; then
        echo "evaluating debian10 for supported DO tarball ..."
        dist='debian10'
        if [[ $is_amd64 == "true" ]]; then
            arch='x64'
        elif [[ $is_arm64 == "true" ]]; then
            arch='arm64'
        elif [[ $is_arm32 == "true" ]]; then
            arch='arm32'
        else
            warn "unsupported arch for DO release asset on Debian10. Supported: amd64 arm64 arm32"
            return 1
        fi
    elif [[ $os_lowercase == "ubuntu" ]]; then
        echo "evaluating ubuntu for supported DO tarball ..."

        if [[ $VER == "18.04" ]]; then
            dist='ubuntu1804'
        elif [[ $VER == "20.04" ]]; then
            dist='ubuntu2004'
        else
            warn "unsupported ubuntu version: $VER. Supported: 18.04 20.04"
            return 1
        fi

        if [[ $dist != '' ]]; then
            if [[ $is_amd64 == "true" ]]; then
                arch='x64'
            elif [[ $is_arm64 == "true" ]]; then
                arch='arm64'
            else
                warn "unsupported arch for DO release asset on ${dist}. Supported: amd64 arm64"
                return 1
            fi
        fi
    fi

    if [[ $dist != '' && $arch != '' ]]; then
        tarball_filename="${dist}_${arch}-packages.tar"
        do_release_tarball_url="https://github.com/microsoft/do-client/releases/download/${do_ref}/${tarball_filename}"

        if [[ ! -e $do_dir ]]; then
            echo "creating $do_dir dir..."
            mkdir -p "$do_dir" || return
        fi

        # v0.9.0 DO has libboost-filesystem and libboost-system deps
        echo "Installing libboost-filesystem-dev and libboost-system-dev DO deps for ${dist} ..."
        $SUDO apt-get install -y libboost-filesystem-dev libboost-system-dev || return

        echo "wget DO release tarball from $do_release_tarball_url ..."
        wget -P "$do_dir" "${do_release_tarball_url}" || return

        echo "extracting $tarball_filename tarball ..."
        pushd "$do_dir" || return
        ls -latr || return
        tar -xf "$tarball_filename" || return

        echo "apt-get installing DO .deb ..."
        $SUDO apt-get install -y ./deliveryoptimization-agent_*.deb ./libdeliveryoptimization_*.deb ./libdeliveryoptimization-dev*.deb || return
        popd || return
    fi

    return 0
}

do_install_do() {
    echo "Installing DO ..."

    # Skip DO installation on Ubuntu 24.04 and newer
    if [[ $OS == "Ubuntu" && $VER == "24.04" ]]; then
        echo "Skipping DO installation on Ubuntu 24.04 (not supported)"
        return 0
    fi

    # Skip DO installation on Debian 13 (trixie) - not yet supported by DO
    if [[ $OS == "Debian" && $VER == "13" ]]; then
        echo "Skipping DO installation on Debian 13 (not yet supported)"
        return 0
    fi

    local do_dir=$work_folder/do
    if [[ -d $do_dir ]]; then
        $SUDO rm -rf "$do_dir" || return
    fi

    if [[ $install_packages == "true" || $install_packages_only == "true" ]]; then
        if do_install_do_release_tarball; then
            echo "Install libdeliveryoptimization from tarball succeeded!"
            return 0
        fi
    fi

    if [[ $keep_source_code != "true" ]]; then
        $SUDO rm -rf "$do_dir" || return
    elif [[ -d $do_dir ]]; then
        warn "$do_dir already exists! Skipping DO."
        return 0
    fi

    echo -e "Building DO ...\n\tBranch: $do_ref\n\tFolder: $do_dir"
    mkdir -p "$do_dir" || return
    pushd "$do_dir" > /dev/null || return

    local do_url
    if [[ $use_ssh == "true" ]]; then
        do_url=git@github.com:Microsoft/do-client.git
    else
        do_url=https://github.com/Microsoft/do-client.git
    fi

    git clone --recursive --single-branch --branch "$do_ref" --depth 1 "$do_url" . || return

    bootstrap_file=$do_dir/build/scripts/bootstrap.sh
    chmod +x "$bootstrap_file" || return
    $SUDO "$bootstrap_file" --install build || return

    mkdir cmake || return
    pushd cmake > /dev/null || return

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
    popd > /dev/null || return
    popd > /dev/null || return
}

do_install_azure_storage_sdk() {
    echo "Installing azure-storage-sdk"

    if is_dep_installed "azure-storage-sdk" "$azure_storage_sdk_tag_ref"; then
        echo "Azure Storage SDK ($azure_storage_sdk_tag_ref) already installed. Skipping..."
        return 0
    fi

    local azure_storage_sdk_dir=$work_folder/azure_storage_sdk_dir

    if [[ -d $azure_storage_sdk_dir ]]; then
        $SUDO rm -rf "$azure_storage_sdk_dir" || return
    fi

    local azure_storage_sdk_url
    if [[ $use_ssh == "true" ]]; then
        azure_storage_sdk_url=git@github.com:Azure/azure-sdk-for-cpp.git
    else
        azure_storage_sdk_url=https://github.com/Azure/azure-sdk-for-cpp.git
    fi

    echo -e "Building Azure Storage SDK ...\n\tBranch: $azure_storage_sdk_branch_ref\n\t Folder: $azure_storage_sdk_dir"
    mkdir -p "$azure_storage_sdk_dir" || return
    pushd "$azure_storage_sdk_dir" > /dev/null || return
    git clone --recursive --single-branch --branch "$azure_storage_sdk_branch_ref" "$azure_storage_sdk_url" . || return

    git checkout tags/"$azure_storage_sdk_tag_ref"

    # Apply patch to fix missing cstdint include for GCC 12+ (Ubuntu 24.04, Debian 12)
    # Check GCC version and apply patch only if GCC >= 12
    local gcc_version
    gcc_version=$(gcc -dumpversion | cut -d. -f1)

    if [[ $gcc_version -ge 12 ]]; then
        local patch_file="$script_dir/patches/azure-storage-sdk-base64-cstdint.patch"
        if [[ -f $patch_file ]]; then
            echo "Detected GCC $gcc_version (>= 12), applying patch to fix base64.cpp compilation issue..."
            git apply "$patch_file" || {
                warn "Failed to apply patch, build may fail on GCC $gcc_version"
            }
        else
            warn "Patch file not found at $patch_file, build may fail on GCC $gcc_version"
        fi
    else
        echo "GCC $gcc_version detected, patch not needed (only required for GCC >= 12)"
    fi

    local azure_storage_sdk_cmake_options=""

    if [[ $keep_source_code == "true" ]]; then
        # If source is wanted, presumably samples and symbols are useful as well.
        azure_storage_sdk_cmake_options+=("-DCMAKE_BUILD_TYPE:STRING=Debug")
    else
        azure_storage_sdk_cmake_options+=("-DCMAKE_BUILD_TYPE:STRING=Release")
    fi

    cmake "${azure_storage_sdk_cmake_options[@]}" . || return

    cmake --build . || return
    $SUDO cmake --build . --target install || return

    popd > /dev/null || return

    mark_dep_installed "azure-storage-sdk" "$azure_storage_sdk_tag_ref"
}

do_install_delta() {
    echo "Installing iot-hub-device-update-delta library ..."

    # Compute effective_delta_ref early so we can check the stamp.
    local OS VER
    OS=$(lsb_release --short --id 2> /dev/null || echo "Unknown")
    VER=$(lsb_release --short --release 2> /dev/null || echo "0")
    local effective_delta_ref=$delta_ref
    if [[ $delta_ref == "main" ]]; then
        effective_delta_ref="feature/vnext-delta"
    fi

    if is_dep_installed "delta" "$effective_delta_ref"; then
        echo "Delta library ($effective_delta_ref) already installed. Skipping..."
        return 0
    fi

    local delta_dir=$work_folder/iot-hub-device-update-delta
    if [[ -d $delta_dir ]]; then
        $SUDO rm -rf "$delta_dir" || return
    fi

    local delta_url
    if [[ $use_ssh == "true" ]]; then
        delta_url=git@github.com:Azure/iot-hub-device-update-delta.git
    else
        delta_url=https://github.com/Azure/iot-hub-device-update-delta.git
    fi

    # Override delta_ref for distros with strict GCC that rejects the 'main' branch code.
    # The 'main' branch uses 'enum class algorithm : uint32_t' which fails on GCC 12+.
    # The feature/vnext-delta and adu/debian/12/amd64 branches use 'enum adu_algorithm' instead.
    if [[ $delta_ref == "main" ]]; then
        echo "Overriding delta_ref from 'main' to '$effective_delta_ref' for GCC compatibility"
    fi

    echo -e "Building iot-hub-device-update-delta library ...\n\tBranch: $effective_delta_ref\n\tFolder: $delta_dir"
    mkdir -p "$delta_dir" || return
    pushd "$delta_dir" > /dev/null || return
    git clone --recursive --single-branch --branch "$effective_delta_ref" --depth 1 "$delta_url" . || return

    # Patch dumpextfs CMakeLists.txt to link com_err (required by libext2fs static lib)
    local dumpextfs_cmake="$delta_dir/src/native/tools/dumpextfs/CMakeLists.txt"
    if [[ -f $dumpextfs_cmake ]] && ! grep -q "com_err" "$dumpextfs_cmake"; then
        echo "Patching dumpextfs CMakeLists.txt to add com_err linkage..."
        sed -i 's/pkg_check_modules(E2FSPROGS REQUIRED ext2fs)/pkg_check_modules(E2FSPROGS REQUIRED ext2fs)\npkg_check_modules(COM_ERR REQUIRED com_err)/' "$dumpextfs_cmake"
        sed -i 's/target_include_directories(dumpextfs PRIVATE ${E2FSPROGS_INCLUDE_DIRS})/target_include_directories(dumpextfs PRIVATE ${E2FSPROGS_INCLUDE_DIRS} ${COM_ERR_INCLUDE_DIRS})/' "$dumpextfs_cmake"
        sed -i 's/target_link_libraries(dumpextfs PRIVATE ${E2FSPROGS_LIBRARIES})/target_link_libraries(dumpextfs PRIVATE ${E2FSPROGS_LIBRARIES} ${COM_ERR_LIBRARIES})/' "$dumpextfs_cmake"
    fi

    # Patch recompress CMakeLists.txt to link libconfig (required by libconfig++ static lib)
    local recompress_cmake="$delta_dir/src/native/tools/recompress/CMakeLists.txt"
    if [[ -f $recompress_cmake ]] && ! grep -q 'LIBCONFIG_C' "$recompress_cmake"; then
        echo "Patching recompress CMakeLists.txt to add libconfig C linkage..."
        sed -i 's/pkg_check_modules(LIBCONFIG REQUIRED libconfig++)/pkg_check_modules(LIBCONFIG REQUIRED libconfig++)\npkg_check_modules(LIBCONFIG_C REQUIRED libconfig)/' "$recompress_cmake"
        sed -i 's/target_link_libraries(recompress PRIVATE ${LIBCONFIG_LIBRARIES} config++)/target_link_libraries(recompress PRIVATE ${LIBCONFIG_LIBRARIES} ${LIBCONFIG_C_LIBRARIES} config++ config)/' "$recompress_cmake"
    fi

    # Install system dependencies required by delta library
    echo "Installing delta library system dependencies..."
    $SUDO apt-get update || return

    # Determine the appropriate GCC version based on distro
    local OS VER gcc_ver
    OS=$(lsb_release --short --id)
    VER=$(lsb_release --short --release)
    if [[ $OS == "Debian" && $VER == "12" ]]; then
        gcc_ver="12"
    elif [[ ($OS == "Debian" && $VER == "11") || ($OS == "Ubuntu" && $VER == "20.04") || ($OS == "Ubuntu" && $VER == "22.04") ]]; then
        gcc_ver="10"
    else
        # Default to system GCC (no specific version suffix)
        gcc_ver=""
    fi

    echo "Using GCC version: ${gcc_ver:-system default}"

    if [[ -n $gcc_ver ]]; then
        # shellcheck disable=SC2086
        $SUDO apt-get install --yes curl zip unzip tar gcc "gcc-${gcc_ver}" g++ "g++-${gcc_ver}" autoconf autopoint ninja-build pkg-config build-essential libtool cmake zlib1g-dev || return
    else
        $SUDO apt-get install --yes curl zip unzip tar gcc g++ autoconf autopoint ninja-build pkg-config build-essential libtool cmake zlib1g-dev || return
    fi

    # Setup gcc/g++ alternatives (only if specific version was installed)
    if [[ -n $gcc_ver ]]; then
        echo "Setting up gcc/g++ alternatives..."
        $SUDO update-alternatives --install /usr/bin/gcc gcc "/usr/bin/gcc-${gcc_ver}" 20 || true
        $SUDO update-alternatives --install /usr/bin/g++ g++ "/usr/bin/g++-${gcc_ver}" 20 || true
    fi

    # Setup VCPKG for delta library dependencies
    echo "Setting up VCPKG for delta library..."
    local vcpkg_root=$work_folder/vcpkg
    local build_type="Release"

    # Auto-detect architecture for vcpkg triplet
    local arch
    arch=$(uname -m)
    local vcpkg_triplet
    local vcpkg_arch
    case "$arch" in
    x86_64 | amd64)
        vcpkg_triplet="x64-linux"
        vcpkg_arch="x64"
        ;;
    aarch64 | arm64)
        vcpkg_triplet="arm64-linux"
        vcpkg_arch="arm64"
        ;;
    armv7l | armhf)
        vcpkg_triplet="arm-linux"
        vcpkg_arch="arm"
        ;;
    *)
        echo "Warning: Unknown architecture '$arch', defaulting to x64-linux"
        vcpkg_triplet="x64-linux"
        vcpkg_arch="x64"
        ;;
    esac
    echo "Detected architecture: $arch -> using triplet: $vcpkg_triplet"

    if [[ $keep_source_code == "true" ]]; then
        build_type="Debug"
    fi

    # Pin to a known-good vcpkg release to avoid breakage from HEAD changes.
    local vcpkg_commit="e0edebd1dc2d03cf7d02349df91de74ef4d0c00e" # 2026.02.27

    # Clone and bootstrap vcpkg if needed
    if [ ! -d "$vcpkg_root" ]; then
        echo "Cloning vcpkg (pinned to $vcpkg_commit)..."
        git clone https://github.com/microsoft/vcpkg "$vcpkg_root" || return
    fi

    pushd "$vcpkg_root" > /dev/null || return
    git fetch origin || true
    git checkout "$vcpkg_commit" || return
    ./bootstrap-vcpkg.sh || return
    popd > /dev/null || return

    # Create triplet if it doesn't exist (community triplet may not be present)
    local triplet_file="$vcpkg_root/triplets/community/$vcpkg_triplet.cmake"
    if [ ! -f "$triplet_file" ]; then
        echo "Creating $vcpkg_triplet triplet..."
        mkdir -p "$vcpkg_root/triplets/community" || return
        cat > "$triplet_file" << EOF
set(VCPKG_TARGET_ARCHITECTURE $vcpkg_arch)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)
EOF
    fi

    # Set environment variables to force classic mode and avoid conflicts with ADU's vcpkg.json
    export VCPKG_ROOT="$vcpkg_root"
    export VCPKG_FEATURE_FLAGS="-manifests"

    # Install dependencies using classic mode with --classic flag
    echo "Installing vcpkg dependencies for delta library..."
    local overlay_ports="$delta_dir/vcpkg/ports"

    # Helper function for vcpkg install with classic mode
    vcpkg_install_classic() {
        local pkg=$1
        echo "Installing $pkg:$vcpkg_triplet..."
        "$vcpkg_root/vcpkg" install "$pkg:$vcpkg_triplet" \
            --classic \
            --overlay-ports="$overlay_ports" \
            --overlay-triplets="$vcpkg_root/triplets/community" \
            --x-install-root="$vcpkg_root/installed" || return 1
    }

    # Install required packages
    vcpkg_install_classic zlib || return
    vcpkg_install_classic zstd || return
    vcpkg_install_classic bzip2 || return
    vcpkg_install_classic gtest || return
    vcpkg_install_classic openssl || return
    vcpkg_install_classic e2fsprogs || return
    vcpkg_install_classic vcpkg-cmake-config || return
    vcpkg_install_classic vcpkg-cmake || return
    vcpkg_install_classic jsoncpp || return
    vcpkg_install_classic libconfig || return
    vcpkg_install_classic fmt || return
    vcpkg_install_classic bsdiff || return

    "$vcpkg_root/vcpkg" integrate install || true
    "$vcpkg_root/vcpkg" list

    # Unset the flag after vcpkg setup
    unset VCPKG_FEATURE_FLAGS

    # Generate bsdiff.pc for pkg-config discovery.
    # The bsdiff vcpkg port only ships a CMake Find module (Findbsdiff.cmake),
    # but the delta library's CMakeLists.txt uses pkg_check_modules(BSDIFF REQUIRED bsdiff).
    # Use ${pcfiledir} for a relocatable prefix (same pattern as zstd's .pc).
    local bsdiff_pc_dir="$vcpkg_root/installed/$vcpkg_triplet/lib/pkgconfig"
    echo "Generating bsdiff.pc for pkg-config discovery..."
    mkdir -p "$bsdiff_pc_dir"
    cat > "$bsdiff_pc_dir/bsdiff.pc" << 'BSDIFF_PC_EOF'
prefix=${pcfiledir}/../..
libdir=${prefix}/lib
includedir=${prefix}/include

Name: bsdiff
Description: Binary diff/patch library
Version: 1.0.0
Libs: -L${libdir} -lbsdiff -ldivsufsort -ldivsufsort64 -lbz2
Cflags: -I${includedir}
BSDIFF_PC_EOF

    # Export PKG_CONFIG_PATH so the delta library CMake build can find vcpkg-installed
    # packages (bsdiff, zstd, etc.) via pkg_check_modules().
    export PKG_CONFIG_PATH="$bsdiff_pc_dir:$vcpkg_root/installed/$vcpkg_triplet/lib/pkgconfig:${PKG_CONFIG_PATH:-}"

    # The delta CMakeLists uses ${BSDIFF_LIBRARIES} / ${ZSTD_LIBRARIES} (bare -l
    # flags) without link_directories for the vcpkg lib path.  LIBRARY_PATH tells
    # the linker where to search.
    export LIBRARY_PATH="$vcpkg_root/installed/$vcpkg_triplet/lib:${LIBRARY_PATH:-}"

    # Build using the delta library's build script
    echo "Building delta library (triplet: $vcpkg_triplet, build type: $build_type)..."
    pushd "$delta_dir/src/native" > /dev/null || return

    chmod +x build.sh || return

    # Set environment variables for build
    export VCPKG_ROOT="$vcpkg_root"
    export VCPKG_OVERLAY_TRIPLETS="$vcpkg_root/triplets/community"
    export VCPKG_FEATURE_FLAGS="-manifests"

    # Skip vcpkg stage since we already installed dependencies above
    # ./build.sh "$vcpkg_triplet" "$build_type" vcpkg || return

    # Run cmake stage
    ./build.sh "$vcpkg_triplet" "$build_type" cmake || return

    # Run build stage
    ./build.sh "$vcpkg_triplet" "$build_type" build || return

    # Unset vcpkg environment variables
    unset VCPKG_OVERLAY_TRIPLETS
    unset VCPKG_FEATURE_FLAGS
    unset PKG_CONFIG_PATH
    unset LIBRARY_PATH

    popd > /dev/null || return

    # Install the built library
    echo "Installing delta library to system..."
    local delta_build_dir="$delta_dir/src/out/native/${vcpkg_triplet}/${build_type}"

    # Install the .deb package if it exists
    # shellcheck disable=SC2144
    if ls "$delta_build_dir/_packages"/*.deb 1> /dev/null 2>&1; then
        echo "Installing from .deb package..."
        $SUDO dpkg -i "$delta_build_dir/_packages"/*.deb || return

        # The .deb package doesn't include the header file, so we need to install it manually
        echo "Installing header file..."
        if [ -f "$delta_dir/src/native/diffs/api/adudiffapi.h" ]; then
            $SUDO cp "$delta_dir/src/native/diffs/api/adudiffapi.h" /usr/include/ || return
            echo "Installed adudiffapi.h to /usr/include/"
        else
            error "Could not find adudiffapi.h in $delta_dir/src/native/diffs/api/"
            return 1
        fi
    else
        # Fallback: manually copy files
        echo "Manually installing library and headers..."

        # Copy the shared library
        if [ -f "$delta_build_dir/bin/libadudiffapi.so" ]; then
            $SUDO cp "$delta_build_dir/bin/libadudiffapi.so"* /usr/local/lib/ || return
            echo "Installed libadudiffapi.so to /usr/local/lib/"
        else
            error "Could not find libadudiffapi.so in $delta_build_dir/bin/"
            return 1
        fi

        # Copy the header file
        if [ -f "$delta_dir/src/native/diffs/api/adudiffapi.h" ]; then
            $SUDO mkdir -p /usr/local/include || return
            $SUDO cp "$delta_dir/src/native/diffs/api/adudiffapi.h" /usr/local/include/ || return
            echo "Installed adudiffapi.h to /usr/local/include/"
        else
            error "Could not find adudiffapi.h in $delta_dir/src/native/diffs/api/"
            return 1
        fi

        # Update library cache
        $SUDO ldconfig || return
        echo "Updated library cache"
    fi

    popd > /dev/null || return

    mark_dep_installed "delta" "$effective_delta_ref"

    if [[ $keep_source_code != "true" ]]; then
        $SUDO rm -rf "$delta_dir" || return
        $SUDO rm -rf "$vcpkg_root" || return
    fi
}

do_install_cmake_from_source() {
    local ret_value
    local cmake_src_url
    local cmake_tar_path
    local cmake_dir_path
    local maj_min_ver=

    if [[ $install_cmake_version != "$supported_cmake_version" ]]; then
        warn "Using unsupported cmake version ${install_cmake_version}!"
    fi

    echo "Building CMake ${install_cmake_version} from source ..."

    local tarball_name="cmake-${install_cmake_version}"
    local tarball_filename="cmake-${install_cmake_version}.tar.gz"
    maj_min_ver=$(echo "$install_cmake_version" | sed -E 's#([0-9]+\.[0-9]+)\.[0-9]+#\1#g') # e.g. 3.23.2 => 3.23

    cmake_src_url="https://cmake.org/files/v${maj_min_ver}/${tarball_filename}"
    cmake_tar_path="$work_folder/${tarball_filename}"
    if [[ -f $cmake_tar_path ]]; then
        $SUDO rm -rf "$cmake_tar_path" || return 1
    fi

    cmake_dir_path="$work_folder/${tarball_name}"
    if [[ -d $cmake_dir_path ]]; then
        $SUDO rm -rf "$cmake_dir_path" || return 1
    fi

    mkdir -p "$cmake_dir_path"
    ret_value=$?
    if [ $ret_value -ne 0 ]; then
        error "Failed to make dir '${cmake_dir_path}' with exit code: ${ret_value}"
        return $ret_value
    fi

    echo "Fetching source tarball '$cmake_src_url' -> '$work_folder' ..."
    wget -P "$work_folder" "$cmake_src_url" > "$cmake_dir_path/wget.log" 2>&1
    ret_value=$?
    if [ $ret_value -ne 0 ]; then
        error "wget of ${cmake_src_url} failed with exit code ${ret_value}"
        return $ret_value
    fi

    echo "Expanding source tarball '$cmake_tar_path' ..."
    tar -xzvf "$cmake_tar_path" -C "$work_folder" > "$cmake_dir_path/tar.log" 2>&1 || return

    pushd "$cmake_dir_path" > /dev/null || return

    echo "Running 'bootstrap' ..."
    $SUDO ./bootstrap --verbose --no-qt-gui --prefix="${cmake_prefix}" > "${cmake_dir_path}/bootstrap.log" 2>&1
    ret_value=$?
    if [ $ret_value -ne 0 ]; then
        error "bootstrap --prefix=${cmake_prefix} failed with exit code ${ret_value}"
        return $ret_value
    fi

    echo "Running 'make' ..."
    $SUDO make > "$cmake_dir_path/make.log" 2>&1 || return

    popd > /dev/null || return

    $SUDO ln -sf "${cmake_prefix}/${tarball_name}" "$cmake_dir_symlink"
    ret_value=$?
    if [ $ret_value -ne 0 ]; then
        error "Failed to create cmake symlink at $cmake_dir_symlink"
        return $ret_value
    fi
}

do_install_cmake_from_installer() {
    local arch="$1"
    shift

    local ret_value

    if [[ $install_cmake_version != "$supported_cmake_version" ]]; then
        warn "Using unsupported cmake version ${install_cmake_version}!"
    fi

    local cmake_installer_sh="cmake-${install_cmake_version}-linux-${arch}.sh"
    local fullpath_cmake_installer_sh="${work_folder}/${cmake_installer_sh}"
    if [[ -f $fullpath_cmake_installer_sh ]]; then
        $SUDO rm "$fullpath_cmake_installer_sh" || return 1
    fi

    local cmake_installer_url="https://github.com/Kitware/CMake/releases/download/v${install_cmake_version}/${cmake_installer_sh}"

    $SUDO wget -P "$work_folder" "$cmake_installer_url"
    ret_value=$?
    if [ $ret_value -ne 0 ]; then
        error "wget failed with exit code ${ret_value}"
        return $ret_value
    fi

    $SUDO chown "$(id -un)":"$(id -gn)" "${fullpath_cmake_installer_sh}"
    chmod u+x "${fullpath_cmake_installer_sh}"
    "${fullpath_cmake_installer_sh}" --include-subdir --skip-license --prefix="${cmake_prefix}"
    ret_value=$?
    if [ $ret_value -ne 0 ]; then
        error "${fullpath_cmake_installer_sh} failed with exit code ${ret_value}"
        return $ret_value
    fi

    $SUDO rm "$fullpath_cmake_installer_sh" || return 1

    ln -sf "$cmake_installer_dir" "$cmake_dir_symlink"
    ret_value=$?
    if [ $ret_value -ne 0 ]; then
        error "Failed to create cmake symlink at $cmake_dir_symlink"
        return $ret_value
    fi
}

do_install_shellcheck() {
    local shellcheck_version=''
    if [ -x "${work_folder}/deviceupdate-shellcheck" ]; then
        shellcheck_version=$("${work_folder}/deviceupdate-shellcheck" --version | grep -i -e '^version:' | awk '{ print $2 }')
    fi

    if [[ $shellcheck_version == "$supported_shellcheck_version" ]]; then
        echo "${work_folder}/deviceupdate-shellcheck at version ${shellcheck_version} already exists. Skipping install..."
        return 0
    fi

    local arch=''
    if [[ $is_arm64 == "true" ]]; then
        arch='aarch64'
    elif [[ $is_amd64 == "true" ]]; then
        arch='x86_64'
    fi

    local base_url='https://github.com/koalaman/shellcheck'
    local scver="$supported_shellcheck_version"
    if [[ $arch == '' ]]; then
        echo "Building shellcheck ${scver} from source..."
        $SUDO apt install --yes cabal-install || return 1
        cabal update || return 1

        local tarball_filename="v${scver}.tar.gz"
        wget -P "$work_folder" "${base_url}/archive/refs/tags/${tarball_filename}" || return 1
        tar -xzvf "$work_folder/$tarball_filename" -C "$work_folder" || return 1

        pushd "${work_folder}/shellcheck-${scver}" > /dev/null || return 1
        cabal install
        local ret_val=$?
        popd || return 1

        if [[ $ret_val != 0 ]]; then
            return $ret_val
        fi

        $SUDO rm "work_folder/$tarball_filename" || return 1

        ln -sf "${HOME}/.cabal/bin/shellcheck" "${work_folder}/deviceupdate-shellcheck" || return 1
    else
        echo "Installing shellcheck ${scver} from pre-built binaries..."
        local tar_filename="shellcheck-v${scver}.linux.${arch}.tar.xz"

        if [[ -f $tar_filename ]]; then
            $SUDO rm $tar_filename || return 1
        fi

        wget -P "$work_folder" "${base_url}/releases/download/v${scver}/${tar_filename}" || return 1
        tar -xf "$work_folder/$tar_filename" -C "$work_folder" || return 1

        $SUDO rm "$work_folder/$tar_filename" || return 1

        ln -sf "${work_folder}/shellcheck-v0.8.0/shellcheck" "${work_folder}/deviceupdate-shellcheck" || return 1
    fi
}

determine_machine_architecture() {
    local arch=''
    arch="$(uname -m)"
    local ret_val=$?
    if [[ $ret_val != 0 ]]; then
        error "Failed to get cpu architecture."
        return 1
    else
        if [[ $arch == aarch64* || $arch == armv8* ]]; then
            is_arm64=true
        elif [[ $arch == armv7* || $arch == 'arm' ]]; then
            is_arm32=true
        elif [[ $arch == 'x86_64' || $arch == 'amd64' ]]; then
            is_amd64=true
        else
            error "Machine architecture '$arch' is not supported."
            return 1
        fi
    fi
}

determine_distro_and_arch() {
    # Checking distro name and version
    if [ -r /etc/os-release ]; then
        # freedesktop.org and systemd
        OS=$(grep "^ID\s*=\s*" /etc/os-release | sed -e "s/^ID\s*=\s*//")
        VER=$(grep "^VERSION_ID=" /etc/os-release | sed -e "s/^VERSION_ID=//")
        VER=$(sed -e 's/^"//' -e 's/"$//' <<< "$VER")
    elif type lsb_release > /dev/null 2>&1; then
        # linuxbase.org
        OS=$(lsb_release -si)
        VER=$(lsb_release -sr)
    elif [ -f /etc/lsb-release ]; then
        # For some versions of Debian/Ubuntu without lsb_release command
        OS=$(grep DISTRIB_ID /etc/lsb-release | awk -F'=' '{ print $2 }')
        VER=$(grep DISTRIB_RELEASE /etc/lsb-release | awk -F'=' '{ print $2 }')
    elif [ -f /etc/debian_version ]; then
        # Older Debian/Ubuntu/etc.
        OS=Debian
        VER=$(cat /etc/debian_version)
    else
        # Fall back to uname, e.g. "Linux <version>", also works for BSD, etc.
        OS=$(uname -s)
        VER=$(uname -r)
    fi

    # Convert OS to lowercase
    OS="$(echo "$OS" | tr '[:upper:]' '[:lower:]')"

    determine_machine_architecture || return 1
}

do_list_all_deps() {
    declare -a deps_set=()
    deps_set+=("${aduc_packages[@]}")
    deps_set+=("${compiler_packages[@]}")
    deps_set+=("${static_analysis_packages[@]}")
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
    --install-azure-storage-sdk)
        install_azure_storage_sdk=true
        ;;
    --azure-iot-sdk-ref)
        shift
        azure_sdk_ref=$1
        ;;
    --install-catch2)
        install_catch2=true
        ;;
    --install-cmake)
        install_cmake=true
        ;;
    --cmake-prefix)
        shift
        cmake_prefix=$1
        ;;
    --cmake-version)
        shift
        install_cmake_version=$1
        ;;
    --cmake-force-source)
        cmake_force_source=true
        ;;
    --install-shellcheck)
        install_shellcheck=true
        ;;
    --install-valgrind)
        install_valgrind=true
        # Check if next argument is a method
        if [[ $2 != "" && $2 != -* ]]; then
            shift
            valgrind_install_method=$1
            if [[ ! $valgrind_install_method =~ ^(apt|source)$ ]]; then
                error "Invalid --install-valgrind method '$valgrind_install_method'. Valid options: apt, source"
                $ret 1
            fi
        fi
        ;;
    --install-githooks)
        install_githooks=true
        ;;
    --catch2-ref)
        shift
        catch2_ref=$1
        ;;
    --install-swupdate)
        install_swupdate=true
        ;;
    --swupdate-ref)
        shift
        swupdate_ref=$1
        ;;
    --install-do)
        install_do=true
        ;;
    --do-ref)
        shift
        do_ref=$1
        ;;
    --install-delta)
        install_delta=true
        ;;
    --delta-ref)
        shift
        delta_ref=$1
        ;;
    -p | --install-packages)
        install_packages=true
        ;;
    --install-packages-only)
        install_packages_only=true
        ;;
    -f | --work-folder)
        shift
        work_folder=$(realpath "$1" 2> /dev/null)
        if [[ -z $work_folder ]]; then
            error "Invalid or inaccessible work folder path: $1"
            $ret 1
        fi
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

# Always setup workfolder with proper ownership, especially for .workspace in repo
if [[ ! -d $work_folder ]]; then
    echo "Creating work folder: $work_folder"
    mkdir -pv "$work_folder" || $ret
fi
# Ensure the work folder has the correct owner (the user running the script, not root)
$SUDO chown "$(id -un)":"$(id -gn)" "$work_folder" || $ret
$SUDO chmod ug+rwx,o= "$work_folder" || $ret

# Ensure the work folder has proper ownership
current_user="$(id -un)"
work_folder_owner="$(stat -c '%U' "$work_folder" 2> /dev/null || echo 'unknown')"
if [[ $work_folder_owner != "$current_user" && $work_folder_owner != "unknown" ]]; then
    echo "Changing ownership of $work_folder to $current_user"
    $SUDO chown -R "$current_user":"$(id -gn)" "$work_folder" || $ret
fi
# Use sudo for chmod in case some files are owned by root or another user
$SUDO chmod -R u+rwx "$work_folder" 2> /dev/null || true

# Set cmake_prefix and cmake_dir_symlink based on work_folder location
cmake_prefix="$work_folder"
cmake_dir_symlink="${work_folder}/deviceupdate-cmake"

# Directory for tracking installed dependency versions.
# Each installed dependency writes a stamp file here so subsequent
# runs can skip re-building when the version hasn't changed.
deps_stamp_dir="$work_folder/.deps-installed"

if [[ -d $du_test_data_dir_path ]]; then
    $SUDO rm -r $du_test_data_dir_path
fi

# Get OS, VER, machine architecture for use in other parts of the script.
determine_distro_and_arch

# If there is no install action specified,
# assume that we want to install all deps.
if [[ $install_all_deps != "true" && $install_aduc_deps != "true" && \
    $install_do != "true" && $install_azure_iot_sdk != "true" && \
    $install_catch2 != "true" && $install_swupdate != "true" && \
    $install_cmake != "true" && $install_shellcheck != "true" && \
    $install_githooks != "true" && $install_delta != "true" ]]; then
    install_all_deps=true
fi

# If --all was specified,
# set all install actions to "true".
if [[ $install_all_deps == "true" ]]; then
    install_aduc_deps=true
    install_do=true
    install_delta=true
    install_cmake=true
    install_shellcheck=true
    install_githooks=true
fi

# Set implied options for aduc deps.
if [[ $install_aduc_deps == "true" ]]; then
    install_cmake=true
    install_azure_iot_sdk=true
    install_catch2=true
    install_azure_storage_sdk=true
    install_delta=true
fi

# Set implied options for packages only.
if [[ $install_packages_only == "true" ]]; then
    install_packages=true
    install_azure_iot_sdk=false
    install_catch2=false
fi

export CC=/usr/bin/gcc
export CXX=/usr/bin/g++
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

# Must be of the form X.Y.Z, where X, Y, and Z are one or more decimal digits.
if [[ $install_cmake_version != "" && ! $install_cmake_version =~ ^[[:digit:]]+.[[:digit:]]+\.[[:digit:]]+ ]]; then
    error "Invalid --cmake-version '${install_cmake_version}'. Valid pattern: digit+.digit+.digit+ e.g. '3.23.2'"
    $ret 1
fi

# First off, install cmake if requested.
if [[ $install_cmake == "true" ]]; then
    cmake_installed=false
    if [[ $is_amd64 == "false" && $is_arm64 == "false" || $cmake_force_source == "true" ]]; then
        if do_install_cmake_from_source; then
            cmake_installed=true
        else
            warn "Failed to install cmake from source. Falling back to system cmake."
        fi
    else
        arch=''
        if [[ $is_amd64 == "true" ]]; then
            arch='x86_64'
        elif [[ $is_arm64 == "true" ]]; then
            arch='aarch64'
        else
            error "Machine architecture is not supported for downloading CMake installer."
            $ret 1
        fi
        cmake_installer_dir="${cmake_prefix}/cmake-${install_cmake_version}-linux-${arch}"

        if [[ -d $cmake_installer_dir && -x "${cmake_dir_symlink}/bin/cmake" ]]; then
            echo "${cmake_installer_dir} already exists. Skipping install of cmake..."
            cmake_installed=true
        else
            if do_install_cmake_from_installer "$arch"; then
                cmake_installed=true
            else
                warn "Failed to install cmake using installer. Falling back to system cmake."
            fi
        fi
    fi
    if [[ $cmake_installed == "true" ]]; then
        cmake_bin="${cmake_dir_symlink}/bin/cmake"
    else
        echo "Using system cmake..."
        cmake_bin="cmake"
    fi
fi

# Write build environment to file for build.sh to source
if [[ $install_cmake == "true" ]]; then
    mkdir -p "$work_folder"
    echo "ADU_CMAKE_BIN=$cmake_bin" > "$work_folder/.build-env"
    echo "Build environment written to $work_folder/.build-env"
fi

# Install git hooks if requested.
if [[ $install_githooks == "true" ]]; then
    if ! do_install_githooks; then
        warn "Failed to install git hooks."
    fi
fi

# Install shellcheck if requested.
if [[ $install_shellcheck == "true" ]]; then
    if ! do_install_shellcheck; then
        warn "Failed to install shellcheck."
    fi
fi

# Install Valgrind if requested.
if [[ $install_valgrind == "true" ]]; then
    if [[ $valgrind_install_method == "apt" ]]; then
        if ! do_install_valgrind_from_apt; then
            error "Failed to install Valgrind from apt."
            $ret 1
        fi
    elif [[ $valgrind_install_method == "source" ]]; then
        if ! do_install_valgrind_from_source; then
            error "Failed to install Valgrind from source."
            $ret 1
        fi
    fi
fi

# Install dependencies from source
if [[ $install_packages_only == "false" ]]; then
    if [[ $install_azure_iot_sdk == "true" ]]; then
        do_install_azure_iot_sdk || $ret
    fi

    if [[ $install_catch2 == "true" ]]; then
        do_install_catch2 || $ret
    fi

    if [[ $install_swupdate == "true" ]]; then
        do_install_swupdate || $ret
    fi

    if [[ $install_do == "true" ]]; then
        do_install_do || $ret
    fi

    if [[ $install_azure_storage_sdk == "true" ]]; then
        do_install_azure_storage_sdk || $ret
    fi

    if [[ $install_delta == "true" ]]; then
        do_install_delta || $ret
    fi
fi

# After installation, it prints out the states of dependencies
if [[ $install_aduc_deps == "true" || $install_do == "true" || $install_packages_only == "true" || $install_packages == "true" ]]; then
    do_list_all_deps || $ret $?
fi
