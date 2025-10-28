#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# install-deps.sh makes it more convenient to install
# dependencies for ADU Agent and Delivery Optimization.
# Some dependencies are installed via packages and
# others are installed from source code.
#
# Key build and development tools installed include:
# - lcov: Code coverage analysis tool for unit testing
# - clang-format: Code formatting tool for consistent style

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

# Setup defaults
install_all_deps=false
install_packages=false
install_packages_only=false
# The folder where source code will be placed
# for building and installing from source.
# Dynamically resolve to script_directory/../deps_tmp/
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEFAULT_WORKFOLDER="$(realpath "$SCRIPT_DIR/../deps_tmp")"
# Use cached work folder if available, otherwise use default
work_folder=${ADUC_WORK_FOLDER:-$DEFAULT_WORKFOLDER}
# Use cached settings if available
keep_source_code=${ADUC_KEEP_SOURCE_CODE:-true}
use_ssh=${ADUC_USE_SSH:-false}

# Environment cache configuration
# Using .adu-dev directory to follow industry pattern (.docker, .github, .vscode, etc.)
ENV_CACHE_DIR="$(realpath "$SCRIPT_DIR/../.adu-dev")"
ENV_CACHE_FILE="$ENV_CACHE_DIR/build.env"

# Load cached environment if available to set defaults
if [[ -f $ENV_CACHE_FILE ]]; then
    echo "Loading cached build environment from $ENV_CACHE_FILE..."
    # shellcheck disable=SC1090
    source "$ENV_CACHE_FILE"
    echo "Cached environment loaded. Command-line options will override cached values."
fi

# ADUC Deps

install_aduc_deps=false
install_azure_iot_sdk=false
# Use cached SDK reference if available
azure_sdk_ref=${AZURE_IOT_SDK_REF:-LTS_08_2023}

# ADUC Diagnostics Deps
azure_storage_sdk_branch_ref=${AZURE_STORAGE_SDK_BRANCH_REF:-main}
azure_storage_sdk_tag_ref=${AZURE_STORAGE_SDK_TAG_REF:-azure-core_1.6.0}
install_azure_storage_sdk=false
# ADUC Test Deps

install_catch2=false
default_catch2_ref=v3.8.0
# Use cached Catch2 reference if available
catch2_ref=${CATCH2_REF:-$default_catch2_ref}
install_swupdate=false
default_swupdate_ref=2021.11
# Use cached SWUpdate reference if available
swupdate_ref=${SWUPDATE_REF:-$default_swupdate_ref}

install_cmake=false
supported_cmake_version='3.23.2'
# Use cached CMake version if available
install_cmake_version=${CMAKE_VERSION:-"$supported_cmake_version"}
cmake_force_source=false
# Use cached CMake prefix if available
cmake_prefix=${CMAKE_PREFIX:-"$work_folder"}
cmake_installer_dir=""
# Use cached CMake directory path if available
cmake_dir_symlink=${ADUC_CMAKE_DIR_PATH:-"/tmp/deviceupdate-cmake"}
# Use cached CMake binary if available
cmake_bin=${CMAKE_BIN:-"cmake"}

install_shellcheck=false
# Use cached shellcheck version if available
supported_shellcheck_version=${SHELLCHECK_VERSION:-'0.8.0'}

install_githooks=false

# Use cached test data directory if available
du_test_data_dir_path=${ADUC_TEST_DATA_DIR:-"/tmp/adu/"}
# DO Deps
default_do_ref=develop
install_do=false
# Use cached DO reference if available
do_ref=${DO_REF:-$default_do_ref}

# catch2 build
#
# used for dependencies like catch2 that will find system default
# (e.g. g++-9) despite g++-10 installed, so use CC and CXX env
# vars that CMake will honor.
# Use cached compiler settings if available
catch2_cc=${CATCH2_CC:-""}
catch2_cxx=${CATCH2_CXX:-""}

# Dependencies packages
aduc_packages=('git' 'make' 'build-essential' 'cmake' 'ninja-build' 'libcurl4-openssl-dev' 'libssl-dev' 'uuid-dev' 'lsb-release' 'curl' 'wget' 'pkg-config' 'libxml2-dev' 'lcov')
static_analysis_packages=('clang' 'clang-tidy' 'cppcheck' 'clang-format')
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
    echo "--cmake-prefix            Set the install path prefix when --install-cmake is used. Default is /tmp."
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
    echo "-p, --install-packages    Indicates that packages should be installed."
    echo "--install-packages-only   Indicates that only packages should be installed and that dependencies should not be installed from source."
    echo ""
    echo "-f, --work-folder <work_folder>   Specifies the folder where source code will be cloned or downloaded."
    echo "                                  Default is script_directory/../deps_tmp (preserves source by default)."
    echo "-k, --keep-source-code <yes|no>  Indicates whether source code should be kept after install from work_folder."
    echo "                                  Arguments: yes, y, no, n (case insensitive). Default is yes."
    echo ""
    echo "--use-ssh                 Use ssh URLs to clone instead of https URLs."
    echo ""
    echo "--list-deps               Show comprehensive dependency analysis report:"
    echo "                          • System package installation status"
    echo "                          • Built-from-source dependency details"
    echo "                          • Version/branch information"
    echo "                          • Build directory status"
    echo "                          • Installation plan based on current options"
    echo "-h, --help                Show this help message."
    echo ""
    echo "Environment Caching:"
    echo "  This script automatically loads previous settings from .adu-dev/build.env"
    echo "  if available (e.g., work folder, SDK versions, compiler paths)."
    echo "  After installation, it caches current settings for sharing with build.sh."
    echo "  Command-line options override cached defaults."
    echo ""
    echo "Examples:"
    echo "  ${BASH_SOURCE[0]} --install-all-deps"
    echo "    # Uses default ../deps_tmp directory and keeps source code"
    echo ""
    echo "  ${BASH_SOURCE[0]} --install-all-deps --work-folder ~/custom-deps --keep-source-code no"
    echo "    # Uses custom directory and deletes source code after build"
    echo ""
    echo "  ${BASH_SOURCE[0]} --install-all-deps --keep-source-code yes"
    echo "    # Explicitly keeps source code (same as default behavior)"
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

    # ADUC packages include build tools, development libraries, and coverage tools:
    #   - lcov: Code coverage analysis tool (works with gcov for test coverage reports)
    $SUDO apt-get install --yes "${aduc_packages[@]}" || return

    # The latest version of gcc available on Debian is gcc-6. We install that version if we are
    # building for Debian, otherwise we install gcc-8 for Ubuntu.
    OS=$(lsb_release --short --id)
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
    # Static analysis packages include:
    #   - clang: C language family frontend for LLVM
    #   - clang-tidy: Clang-based C++ linter tool
    #   - cppcheck: Tool for static analysis of C/C++ code
    #   - clang-format: Tool for formatting C/C++ code (used for generated result.h)
    $SUDO apt-get install --yes "${static_analysis_packages[@]}" || return
}

do_install_azure_iot_sdk() {
    echo "Installing Azure IoT C SDK ..."
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
    git clone --branch $azure_sdk_ref $azure_sdk_url . || return
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

    if [[ $keep_source_code != "true" ]]; then
        $SUDO rm -rf "$azure_sdk_dir" || return
    fi
}

do_install_catch2() {
    echo "Installing Catch2 ..."
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
    git clone --recursive --single-branch --branch $catch2_ref --depth 1 $catch2_url . || return

    mkdir cmake || return
    pushd cmake > /dev/null || return

    CC="$catch2_cc" CXX="$catch2_cxx" "$cmake_bin" .. || return
    CC="$catch2_cc" CXX="$catch2_cxx" "$cmake_bin" --build . || return
    $SUDO "$cmake_bin" --build . --target install || return
    popd > /dev/null || return
    popd > /dev/null || return

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
    git clone --recursive --single-branch --branch $swupdate_ref --depth 1 $swupdate_url . || return

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

    git clone --recursive --single-branch --branch $do_ref --depth 1 $do_url . || return

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
    git clone --recursive --single-branch --branch $azure_storage_sdk_branch_ref $azure_storage_sdk_url . || return

    git checkout tags/$azure_storage_sdk_tag_ref

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

        $SUDO rm "$work_folder/$tarball_filename" || return 1

        # Copy the built binary to work folder
        cp "${HOME}/.cabal/bin/shellcheck" "${work_folder}/deviceupdate-shellcheck" || return 1
    else
        echo "Installing shellcheck ${scver} from pre-built binaries..."
        local tar_filename="shellcheck-v${scver}.linux.${arch}.tar.xz"

        if [[ -f $tar_filename ]]; then
            $SUDO rm $tar_filename || return 1
        fi

        wget -P "$work_folder" "${base_url}/releases/download/v${scver}/${tar_filename}" || return 1
        tar -xf "$work_folder/$tar_filename" -C "$work_folder" || return 1

        $SUDO rm "$work_folder/$tar_filename" || return 1

        # Copy the extracted binary to standardized location
        cp "${work_folder}/shellcheck-v${scver}/shellcheck" "${work_folder}/deviceupdate-shellcheck" || return 1
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
    echo "=================================================================="
    echo "    ADU Dependencies Analysis Report"
    echo "=================================================================="
    echo ""

    # Configuration Summary
    echo "📋 Configuration:"
    echo "  Work Folder: $work_folder"
    echo "  Keep Source Code: $keep_source_code"
    echo "  Install Packages: $install_packages"
    echo "  Install Packages Only: $install_packages_only"
    echo "  Use SSH: $use_ssh"
    echo ""

    # System Packages
    echo "📦 System Packages:"
    echo "------------------------------------------------------------------"
    declare -a deps_set=()
    deps_set+=("${aduc_packages[@]}")
    deps_set+=("${compiler_packages[@]}")
    deps_set+=("${static_analysis_packages[@]}")

    echo "ADU Core Packages:"
    for pkg in "${aduc_packages[@]}"; do
        if dpkg-query -W "$pkg" > /dev/null 2>&1; then
            version=$(dpkg-query -W -f='${Version}' "$pkg" 2> /dev/null)
            echo "  ✅ $pkg ($version)"
        else
            echo "  ❌ $pkg (not installed)"
        fi
    done

    echo ""
    echo "Compiler Packages:"
    for pkg in "${compiler_packages[@]}"; do
        if dpkg-query -W "$pkg" > /dev/null 2>&1; then
            version=$(dpkg-query -W -f='${Version}' "$pkg" 2> /dev/null)
            echo "  ✅ $pkg ($version)"
        else
            echo "  ❌ $pkg (not installed)"
        fi
    done

    echo ""
    echo "Static Analysis Packages:"
    for pkg in "${static_analysis_packages[@]}"; do
        if dpkg-query -W "$pkg" > /dev/null 2>&1; then
            version=$(dpkg-query -W -f='${Version}' "$pkg" 2> /dev/null)
            echo "  ✅ $pkg ($version)"
        else
            echo "  ❌ $pkg (not installed)"
        fi
    done
    echo ""

    # Built-from-Source Dependencies
    echo "🔧 Built-from-Source Dependencies:"
    echo "------------------------------------------------------------------"

    # Azure IoT SDK
    echo "Azure IoT C SDK:"
    echo "  📋 Branch/Tag: $azure_sdk_ref"
    echo "  📂 Build Dir: $work_folder/azure-iot-sdk-c"
    echo "  🌐 Repository: https://github.com/Azure/azure-iot-sdk-c.git"
    if [[ -d "$work_folder/azure-iot-sdk-c" ]]; then
        echo "  ✅ Source code present"
        if [[ -f "$work_folder/azure-iot-sdk-c/cmake/CMakeCache.txt" ]]; then
            echo "  ✅ Build configured"
        else
            echo "  ⚠️  Build not configured"
        fi
    else
        echo "  ❌ Source code not present"
    fi
    echo ""

    # Catch2
    echo "Catch2 Testing Framework:"
    echo "  📋 Branch/Tag: $catch2_ref"
    echo "  📂 Build Dir: $work_folder/catch2"
    echo "  🌐 Repository: https://github.com/catchorg/Catch2.git"
    if [[ -d "$work_folder/catch2" ]]; then
        echo "  ✅ Source code present"
        if [[ -f "$work_folder/catch2/cmake/CMakeCache.txt" ]]; then
            echo "  ✅ Build configured"
        else
            echo "  ⚠️  Build not configured"
        fi
    else
        echo "  ❌ Source code not present"
    fi
    echo ""

    # SWUpdate (Ubuntu specific)
    echo "SWUpdate (Ubuntu 18.04/20.04 only):"
    echo "  📋 Branch/Tag: $swupdate_ref"
    echo "  📂 Build Dir: $work_folder/swupdate"
    echo "  🌐 Repository: https://github.com/sbabic/swupdate.git"
    if lsb_release -a 2> /dev/null | grep -q -e 'Ubuntu 18.04' -e 'Ubuntu 20.04'; then
        echo "  ✅ OS supported for SWUpdate"
        if [[ -d "$work_folder/swupdate" ]]; then
            echo "  ✅ Source code present"
            if [[ -f "$work_folder/swupdate/swupdate" ]]; then
                echo "  ✅ Built binary present"
            else
                echo "  ⚠️  Binary not built"
            fi
        else
            echo "  ❌ Source code not present"
        fi
    else
        echo "  ⏭️  OS not supported (skipped)"
    fi
    echo ""

    # Delivery Optimization
    echo "Delivery Optimization (DO):"
    echo "  📋 Branch/Tag: $do_ref"
    echo "  📂 Build Dir: $work_folder/do"
    echo "  🌐 Repository: https://github.com/Microsoft/do-client.git"
    if [[ -d "$work_folder/do" ]]; then
        echo "  ✅ Source code present"
        if [[ -f "$work_folder/do/cmake/CMakeCache.txt" ]]; then
            echo "  ✅ Build configured"
        else
            echo "  ⚠️  Build not configured"
        fi
    else
        echo "  ❌ Source code not present"
    fi
    echo ""

    # Azure Storage SDK
    echo "Azure Storage SDK for C++:"
    echo "  📋 Branch: $azure_storage_sdk_branch_ref"
    echo "  📋 Tag: $azure_storage_sdk_tag_ref"
    echo "  📂 Build Dir: $work_folder/azure_storage_sdk_dir"
    echo "  🌐 Repository: https://github.com/Azure/azure-sdk-for-cpp.git"
    if [[ -d "$work_folder/azure_storage_sdk_dir" ]]; then
        echo "  ✅ Source code present"
        if [[ -f "$work_folder/azure_storage_sdk_dir/cmake/CMakeCache.txt" ]]; then
            echo "  ✅ Build configured"
        else
            echo "  ⚠️  Build not configured"
        fi
    else
        echo "  ❌ Source code not present"
    fi
    echo ""

    # CMake
    echo "CMake (if building from source):"
    echo "  📋 Version: $install_cmake_version"
    echo "  📂 Build Dir: $work_folder/cmake-$install_cmake_version"
    echo "  🌐 Source: https://cmake.org/files/"
    current_cmake_version=$(cmake --version 2> /dev/null | head -n1 | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+' || echo "not installed")
    echo "  📋 Current Installed: $current_cmake_version"
    if [[ -d "$work_folder/cmake-$install_cmake_version" ]]; then
        echo "  ✅ Source code present"
        if [[ -f "$work_folder/cmake-$install_cmake_version/bin/cmake" ]]; then
            echo "  ✅ Built binary present"
        else
            echo "  ⚠️  Binary not built"
        fi
    else
        echo "  ❌ Source code not present"
    fi
    echo ""

    # Shellcheck
    echo "Shellcheck:"
    echo "  📋 Version: $supported_shellcheck_version"
    echo "  📂 Binary Path: $work_folder/deviceupdate-shellcheck"
    if [[ -f "$work_folder/deviceupdate-shellcheck" ]]; then
        installed_version=$("$work_folder/deviceupdate-shellcheck" --version 2> /dev/null | grep -i -e '^version:' | awk '{ print $2 }' || echo "unknown")
        echo "  ✅ Binary present (v$installed_version)"
    else
        echo "  ❌ Binary not present"
    fi
    echo ""

    # Installation Plan
    echo "🚀 Installation Plan Based on Current Options:"
    echo "------------------------------------------------------------------"
    if [[ $install_all_deps == "true" ]]; then
        echo "  • Install ALL dependencies (system packages + build from source)"
    elif [[ $install_aduc_deps == "true" ]]; then
        echo "  • Install ADU Core dependencies only"
    elif [[ $install_packages_only == "true" ]]; then
        echo "  • Install system packages only (no building from source)"
    elif [[ $install_packages == "true" ]]; then
        echo "  • Install system packages + build from source"
    else
        echo "  • No installation options specified"
    fi

    if [[ $install_cmake == "true" ]]; then
        echo "  • Build CMake from source"
    fi

    if [[ $install_catch2 == "true" ]]; then
        echo "  • Build Catch2 testing framework"
    fi

    if [[ $install_swupdate == "true" ]]; then
        echo "  • Build SWUpdate"
    fi

    if [[ $install_do == "true" ]]; then
        echo "  • Build Delivery Optimization"
    fi

    if [[ $install_azure_storage_sdk == "true" ]]; then
        echo "  • Build Azure Storage SDK"
    fi

    if [[ $install_shellcheck == "true" ]]; then
        echo "  • Install Shellcheck"
    fi

    echo ""
    echo "=================================================================="
    echo "For troubleshooting build failures, check:"
    echo "  • Build logs in respective build directories"
    echo "  • CMakeCache.txt files for configuration issues"
    echo "  • Ensure all system packages are installed"
    echo "  • Verify network connectivity for git clones"
    echo "=================================================================="

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
        shift
        case "$1" in
        yes | y | Y | Yes | YES)
            keep_source_code=true
            ;;
        no | n | N | No | NO)
            keep_source_code=false
            ;;
        *)
            error "Invalid argument for --keep-source-code: '$1'. Valid options: yes, no, y, n"
            $ret 1
            ;;
        esac
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

# Setup workfolder - ensure it exists and has proper permissions
mkdir -pv "$work_folder" || $ret
if [[ $work_folder != "$DEFAULT_WORKFOLDER" ]]; then
    # Only change ownership/permissions for custom work folders
    $SUDO chown "$(id -un)":"$(id -gn)" "$work_folder" || $ret
    chmod ug+rwx,o= "$work_folder" || $ret
fi

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
    $install_githooks != "true" ]]; then
    install_all_deps=true
fi

# If --all was specified,
# set all install actions to "true".
if [[ $install_all_deps == "true" ]]; then
    install_aduc_deps=true
    install_do=true
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
fi

# Set implied options for packages only.
if [[ $install_packages_only == "true" ]]; then
    install_packages=true
    install_azure_iot_sdk=false
    install_catch2=false
fi

export CC=/usr/bin/gcc
export CXX=/usr/bin/g++

# Function to cache environment variables for sharing with build.sh
cache_build_environment() {
    echo "Caching build environment to $ENV_CACHE_FILE..."

    # Create directory if it doesn't exist
    mkdir -p "$ENV_CACHE_DIR"

    # Create .env file with build environment variables
    cat > "$ENV_CACHE_FILE" << EOF
# ADU Build Environment Cache
# Generated by install-deps.sh on $(date)
# This file contains environment variables shared between install-deps.sh and build.sh

# Build directories and paths
ADUC_WORK_FOLDER="$work_folder"
ADUC_DEPS_FOLDER="$work_folder"
ADUC_CMAKE_DIR_PATH="$cmake_dir_symlink"

# Compiler settings
CC="$CC"
CXX="$CXX"
CATCH2_CC="$catch2_cc"
CATCH2_CXX="$catch2_cxx"

# CMake configuration
CMAKE_BIN="$cmake_bin"
CMAKE_VERSION="$install_cmake_version"
CMAKE_PREFIX="$cmake_prefix"

# SDK and library references
AZURE_IOT_SDK_REF="$azure_sdk_ref"
CATCH2_REF="$catch2_ref"
SWUPDATE_REF="$swupdate_ref"
DO_REF="$do_ref"
AZURE_STORAGE_SDK_BRANCH_REF="$azure_storage_sdk_branch_ref"
AZURE_STORAGE_SDK_TAG_REF="$azure_storage_sdk_tag_ref"

# System information
ADUC_OS="$OS"
ADUC_VERSION="$VER"
ADUC_IS_AMD64="$is_amd64"
ADUC_IS_ARM64="$is_arm64"
ADUC_IS_ARM32="$is_arm32"

# Build options
ADUC_KEEP_SOURCE_CODE="$keep_source_code"
ADUC_USE_SSH="$use_ssh"

# Test and validation settings
ADUC_TEST_DATA_DIR="$du_test_data_dir_path"
SHELLCHECK_VERSION="$supported_shellcheck_version"

# Installation flags (for reference)
ADUC_INSTALLED_DEPS="$(date)"
ADUC_INSTALL_ALL_DEPS="$install_all_deps"
ADUC_INSTALL_ADUC_DEPS="$install_aduc_deps"
ADUC_INSTALL_AZURE_IOT_SDK="$install_azure_iot_sdk"
ADUC_INSTALL_AZURE_STORAGE_SDK="$install_azure_storage_sdk"
ADUC_INSTALL_CATCH2="$install_catch2"
ADUC_INSTALL_CMAKE="$install_cmake"
ADUC_INSTALL_DO="$install_do"
ADUC_INSTALL_SHELLCHECK="$install_shellcheck"
ADUC_INSTALL_SWUPDATE="$install_swupdate"
EOF

    echo "Build environment cached successfully."
    echo "  Cache file: $ENV_CACHE_FILE"
    echo "  Use 'source $ENV_CACHE_FILE' in build.sh to load these variables."
}

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
    if [[ $is_amd64 == "false" && $is_arm64 == "false" || $cmake_force_source == "true" ]]; then
        if ! do_install_cmake_from_source; then
            error "Failed to install cmake from source."
            $ret 1
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
        else
            if ! do_install_cmake_from_installer "$arch"; then
                error "Failed to install cmake using installer."
                $ret 1
            fi
        fi
    fi
    cmake_bin="${cmake_dir_symlink}/bin/cmake"
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
fi

# After installation, it prints out the states of dependencies
if [[ $install_aduc_deps == "true" || $install_do == "true" || $install_packages_only == "true" || $install_packages == "true" ]]; then
    do_list_all_deps || $ret $?
fi

# Cache environment variables for sharing with build.sh
cache_build_environment
