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
DEFAULT_WORKFOLDER=/tmp
work_folder=$DEFAULT_WORKFOLDER
keep_source_code=false
use_ssh=false

# ADUC Deps

install_aduc_deps=false
install_azure_iot_sdk=false
azure_sdk_ref=LTS_07_2021_Ref01

# ADUC Test Deps

install_catch2=false
default_catch2_ref=v2.11.0
catch2_ref=$default_catch2_ref
install_swupdate=false
default_swupdate_ref=2021.11
swupdate_ref=$default_swupdate_ref

install_azure_blob_storage_file_upload_utility=false
azure_blob_storage_file_upload_utility_ref=main

install_cmake=false
supported_cmake_version='3.23.2'
install_cmake_version="$supported_cmake_version"
cmake_force_source=false
cmake_prefix="$work_folder"
cmake_installer_dir=""
cmake_dir_symlink="/tmp/deviceupdate-cmake"

install_shellcheck=false
supported_shellcheck_version='0.8.0'

install_githooks=false

du_test_data_dir_path="/tmp/adu/"
# DO Deps
default_do_ref=v1.0.1
install_do=false
do_ref=$default_do_ref

# Dependencies packages
aduc_packages=('git' 'make' 'build-essential' 'cmake' 'ninja-build' 'libcurl4-openssl-dev' 'libssl-dev' 'uuid-dev' 'python2.7' 'lsb-release' 'curl' 'wget' 'pkg-config')
static_analysis_packages=('clang' 'clang-tidy' 'cppcheck')
compiler_packages=("gcc-[68]")

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
    echo "--azure-iot-sdk-ref <ref> Install the Azure IoT C SDK from a specific branch or tag."
    echo "                           Default is public-preview."
    echo "--install-abs-file-upload-utility   Install the Azure Blob Storage File Upload Utility from source."
    echo "--abs-file-upload-utility-ref <ref> Install the Azure Blob Storage File Upload Utility from a specific branch or tag."
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
    echo "                                  Default is /tmp."
    echo "-k, --keep-source-code            Indicates that source code should not be deleted after install from work_folder."
    echo ""
    echo "--use-ssh                 Use ssh URLs to clone instead of https URLs."
    echo "--list-deps               List the states of the dependencies."
    echo "-h, --help                Show this help message."
    echo ""
    echo "Example: ${BASH_SOURCE[0]} --install-all-deps --work-folder ~/adu-linux-client-deps --keep-source-code"
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
}

do_install_azure_iot_sdk() {
    echo "Installing Azure IoT C SDK ..."
    local azure_sdk_dir=$work_folder/azure-iot-sdk-c
    if [[ -d $azure_sdk_dir ]]; then
        $SUDO rm -rf $azure_sdk_dir || return
    fi

    local azure_sdk_url
    if [[ $use_ssh == "true" ]]; then
        azure_sdk_url=git@github.com:Azure/azure-iot-sdk-c.git
    else
        azure_sdk_url=https://github.com/Azure/azure-iot-sdk-c.git
    fi

    echo -e "Building azure-iot-sdk-c ...\n\tBranch: $azure_sdk_ref\n\tFolder: $azure_sdk_dir"
    mkdir -p $azure_sdk_dir || return
    pushd $azure_sdk_dir > /dev/null || return
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
        $SUDO rm -rf $azure_sdk_dir || return
    fi
}

do_install_catch2() {
    echo "Installing Catch2 ..."
    local catch2_dir=$work_folder/catch2
    if [[ -d $catch2_dir ]]; then
        $SUDO rm -rf $catch2_dir || return
    fi

    local catch2_url
    if [[ $use_ssh == "true" ]]; then
        catch2_url=git@github.com:catchorg/Catch2.git
    else
        catch2_url=https://github.com/catchorg/Catch2.git
    fi

    echo -e "Building Catch2 ...\n\tBranch: $catch2_ref\n\tFolder: $catch2_dir"
    mkdir -p $catch2_dir || return
    pushd $catch2_dir > /dev/null || return
    git clone --recursive --single-branch --branch $catch2_ref --depth 1 $catch2_url . || return

    mkdir cmake || return
    pushd cmake > /dev/null || return
    cmake .. || return
    cmake --build . || return
    $SUDO cmake --build . --target install || return
    popd > /dev/null || return
    popd > /dev/null || return

    if [[ $keep_source_code != "true" ]]; then
        $SUDO rm -rf $catch2_dir || return
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
        $SUDO rm -rf $swupdate_dir || return 1
    fi

    local swupdate_url
    if [[ $use_ssh == "true" ]]; then
        swupdate_url=git@github.com:sbabic/swupdate.git
    else
        swupdate_url=https://github.com/sbabic/swupdate.git
    fi

    echo -e "Building SWUpdate ...\n\tBranch: $swupdate_ref\n\tFolder: $swupdate_dir"
    mkdir -p $swupdate_dir || return
    pushd $swupdate_dir > /dev/null || return
    git clone --recursive --single-branch --branch $swupdate_ref --depth 1 $swupdate_url . || return

    popd > /dev/null || return
    echo -e "Customizing SWUpdate build configurations..."
    cp src/deps/swupdate/.config "$swupdate_dir" || return
    pushd $swupdate_dir > /dev/null || return

    echo -r "Building SWUpdate..."
    make || return

    echo -e "Installing SWUpdate..."
    $SUDO make install || return
    popd > /dev/null || return

    if [[ $keep_source_code != "true" ]]; then
        $SUDO rm -rf $swupdate_dir || return 1
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
        tar -xvf "$tarball_filename" || return

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
        $SUDO rm -rf $do_dir || return
    fi

    if [[ $install_packages == "true" || $install_packages_only == "true" ]]; then
        if do_install_do_release_tarball; then
            echo "Install libdeliveryoptimization from tarball succeeded!"
            return 0
        fi
    fi

    if [[ $keep_source_code != "true" ]]; then
        $SUDO rm -rf $do_dir || return
    elif [[ -d $do_dir ]]; then
        warn "$do_dir already exists! Skipping DO."
        return 0
    fi

    echo -e "Building DO ...\n\tBranch: $do_ref\n\tFolder: $do_dir"
    mkdir -p $do_dir || return
    pushd $do_dir > /dev/null || return

    local do_url
    if [[ $use_ssh == "true" ]]; then
        do_url=git@github.com:Microsoft/do-client.git
    else
        do_url=https://github.com/Microsoft/do-client.git
    fi

    git clone --recursive --single-branch --branch $do_ref --depth 1 $do_url . || return

    distro=$OS$VER
    install_do_deps_distro="${distro//./}"

    if [[ $install_do_deps_distro != "" ]]; then
        local bootstrap_file=$do_dir/build/scripts/bootstrap.sh
        chmod +x $bootstrap_file || return
        $SUDO $bootstrap_file --platform "$install_do_deps_distro" --install build || return
    fi

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

    if [[ $keep_source_code != "true" ]]; then
        $SUDO rm -rf $do_dir
    fi
}

do_install_azure_blob_storage_file_upload_utility() {
    echo "Installing azure-blob-storage-file-upload-utility from source."
    local abs_fuu_dir=$work_folder/azure_blob_storage_file_upload_utility

    if [[ -d $abs_fuu_dir ]]; then
        $SUDO rm -rf $abs_fuu_dir || return 1
    fi

    local azure_storage_cpplite_url
    if [[ $use_ssh == "true" ]]; then
        azure_storage_cpplite_url=git@github.com:Azure/azure-blob-storage-file-upload-utility.git
    else
        azure_storage_cpplite_url=https://github.com/Azure/azure-blob-storage-file-upload-utility.git
    fi

    echo -e "Cloning Azure Blob Storage File Upload Uility ...\n\tBranch: $azure_blob_storage_file_upload_utility_ref\n\t Folder: $abs_fuu_dir"
    mkdir -p $abs_fuu_dir || return
    pushd $abs_fuu_dir > /dev/null || return
    git clone --recursive --single-branch --branch $azure_blob_storage_file_upload_utility_ref --depth 1 $azure_storage_cpplite_url . || return

    echo -e "Installing Azure Blob Storage File Upload Utiltiy dependencies..."

    # Note added to make sure that install-deps.sh is executable
    chmod u+x ./scripts/install-deps.sh

    # Note we can skip the azure iot sdk installation because it is guaranteed that it will already be installed.
    ./scripts/install-deps.sh -a --skip-azure-iot-sdk-install

    mkdir cmake || return
    pushd cmake > /dev/null || return

    local azure_blob_storage_file_upload_utility_cmake_options
    if [[ $keep_source_code == "true" ]]; then
        # If source is wanted, presumably samples and symbols are useful as well.
        azure_blob_storage_file_upload_utility_cmake_options+=("-DCMAKE_BUILD_TYPE:STRING=Debug")
    else
        azure_blob_storage_file_upload_utility_cmake_options+=("-DCMAKE_BUILD_TYPE:STRING=Release")
    fi

    echo -e "Building Azure Blob Storage File Upload Uility ...\n\tBranch: $azure_blob_storage_file_upload_utility_ref\n\t"
    cmake "${azure_blob_storage_file_upload_utility_cmake_options[@]}" .. || return

    cmake --build . || return 1
    $SUDO cmake --build . --target install || return

    popd > /dev/null || return
    popd > /dev/null || return

    if [[ $keep_source_code != "true" ]]; then
        $SUDO rm -rf $abs_fuu_dir || return 1
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
        $SUDO rm -rf $cmake_tar_path || return 1
    fi

    cmake_dir_path="$work_folder/${tarball_name}"
    if [[ -d $cmake_dir_path ]]; then
        $SUDO rm -rf $cmake_dir_path || return 1
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
    $SUDO ./bootstrap --verbose --no-qt-gui --prefix=${cmake_prefix} > "${cmake_dir_path}/bootstrap.log" 2>&1
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
    "${fullpath_cmake_installer_sh}" --include-subdir --skip-license --prefix=${cmake_prefix}
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
        echo "${work_folder}/deviceupdate-shellcheck at version ${supported_cmake_version} already exists. Skipping install..."
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

        ln -sf "${HOME}/.cabal/bin/shellcheck" "/tmp/deviceupdate-shellcheck" || return 1
    else
        echo "Installing shellcheck ${scver} from pre-built binaries..."
        local tar_filename="shellcheck-v${scver}.linux.x86_64.tar.xz"

        if [[ -f $tar_filename ]]; then
            $SUDO rm $tar_filename || return 1
        fi

        wget -P "$work_folder" "${base_url}/releases/download/v${scver}/${tar_filename}" || return 1
        tar -xvf "$work_folder/$tar_filename" -C "$work_folder" || return 1

        $SUDO rm "$work_folder/$tar_filename" || return 1

        ln -sf "${work_folder}/shellcheck-v0.8.0/shellcheck" "/tmp/deviceupdate-shellcheck" || return 1
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

# setup workfolder if different from default
if [[ $work_folder != "$DEFAULT_WORKFOLDER" ]]; then
    mkdir -pv "$work_folder" || $ret
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

    if [[ $install_azure_blob_storage_file_upload_utility == "true" ]]; then
        do_install_azure_blob_storage_file_upload_utility || $ret
    fi
fi

# After installation, it prints out the states of dependencies
if [[ $install_aduc_deps == "true" || $install_do == "true" || $install_packages_only == "true" || $install_packages == "true" ]]; then
    do_list_all_deps || $ret $?
fi
