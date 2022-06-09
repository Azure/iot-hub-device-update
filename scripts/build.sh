#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Ensure that getopt starts from first option if ". <script.sh>" was used.
OPTIND=1

# Ensure we dont end the user's terminal session if invoked from source (".").
if [[ $0 != "${BASH_SOURCE[0]}" ]]; then
    ret=return
else
    ret=exit
fi

warn() { echo -e "\033[1;33mWarning:\033[0m $*" >&2; }

error() { echo -e "\033[1;31mError:\033[0m $*" >&2; }

header() { echo -e "\e[4m\e[1m\e[1;32m$*\e[0m"; }

bullet() { echo -e "\e[1;34m*\e[0m $*"; }

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
root_dir=$script_dir/..

build_clean=false
build_documentation=false
build_packages=false
platform_layer="linux"
content_handlers="microsoft/swupdate,microsoft/apt,microsoft/simulator"
build_type=Debug
adu_log_dir=""
default_log_dir=/var/log/adu
output_directory=$root_dir/out
build_unittests=false
declare -a static_analysis_tools=()
log_lib="zlog"
install_prefix=/usr/local
install_adu=false
cmake_dir_path=/tmp/cmake-3.10.2

print_help() {
    echo "Usage: build.sh [options...]"
    echo "-c, --clean                           Does a clean build."
    echo "-t, --type <build_type>               The type of build to produce. Passed to CMAKE_BUILD_TYPE. Default is Debug."
    echo "                                      Options: Release Debug RelWithDebInfo MinSizeRel"
    echo "-d, --build-docs                      Builds the documentation."
    echo "-u, --build-unit-tests                Builds unit tests."
    echo "--build-packages                      Builds and packages the client in various package formats e.g debian."
    echo "-o, --out-dir <out_dir>               Sets the build output directory. Default is out."
    echo "-s, --static-analysis <tools...>      Runs static analysis as part of the build."
    echo "                                      Tools is a comma delimited list of static analysis tools to run at build time."
    echo "                                      Tools: clang-tidy cppcheck cpplint iwyu lwyu (or all)"
    echo ""
    echo "-p, --platform-layer <layer>          Specify the platform layer to build/use. Default is linux."
    echo "                                      Option: linux"
    echo ""
    echo "--log-lib <log_lib>                   Specify the logging library to build/use. Default is zlog."
    echo "                                      Options: zlog xlog"
    echo ""
    echo "-l, --log-dir <log_dir>               Specify the directory where the ADU Agent will write logs."
    echo "                                      Only valid for logging libraries that support file logging."
    echo ""
    echo "--install-prefix <prefix>             Install prefix to pass to CMake."
    echo ""
    echo "--install                             Install the following ADU components."
    echo "                                          From source: adu-agent.service & adu-swupdate.sh."
    echo "                                          From build output directory: AducIotAgent & adu-shell."
    echo ""
    echo "--cmake-path                          Specify the cmake path that we want to use to build ADU."
    echo "                                      Default will be the path same as install-deps.sh cmake_dir_path"
    echo ""
    echo "-h, --help                            Show this help message."
}

copyfile_exit_if_failed() {
    bullet "Copying $1 to $2"
    cp "$1" "$2"
    ret_val=$?
    if [[ $ret_val != 0 ]]; then
        error "failed to copy $1 to $2 (exit code:$ret_val)"
        exit $ret_val
    fi
}

install_adu_components() {
    adu_lib_dir=/usr/lib/adu
    adu_data_dir=/var/lib/adu

    # Install adu components on this local development machine.
    header "Installing ADU components..."
    bullet "Create 'adu' user and 'adu' group..."

    groupadd --system adu
    useradd --system -p '' -g adu --no-create-home --shell /sbin/false adu

    bullet "Add current user ('$USER') to 'adu' group, to allow launching adu-agent (for testing purposes only)"
    usermod -a -G adu "$USER"
    bullet "Current user info:"
    id

    copyfile_exit_if_failed "$output_directory/bin/AducIotAgent" /usr/bin

    mkdir -p $adu_lib_dir

    copyfile_exit_if_failed "$output_directory/bin/adu-shell" $adu_lib_dir
    copyfile_exit_if_failed "$root_dir/src/adu-shell/scripts/adu-swupdate.sh" "$adu_lib_dir"

    # Setup directories owner and/or permissions

    # Logs directory
    mkdir -p "$adu_log_dir"

    chown adu:adu "$adu_log_dir"
    chmod u=rwx,g=rx "$adu_log_dir"

    # Data directory
    mkdir -p "$adu_data_dir"

    chown adu:adu "$adu_data_dir"
    chmod u=rwx,g=rx "$adu_data_dir"

    # Set the file permissions
    chown root:adu "$adu_lib_dir/adu-shell"
    chmod u=rwxs,g=rx,o= "$adu_lib_dir/adu-shell"
    chmod u=rwx,g=rx,o=rx "$adu_lib_dir/adu-swupdate.sh"

    # Only install adu-agent service on system that support systemd.
    systemd_system_dir=/usr/lib/systemd/system/
    if [ -d "$systemd_system_dir" ]; then
        bullet "Install adu-agent systemd daemon..."
        copyfile_exit_if_failed "$root_dir/daemon/adu-agent.service" "$systemd_system_dir"

        systemctl daemon-reload
        systemctl enable adu-agent
        systemctl restart adu-agent
    else
        warn "Directory $systemd_system_dir does not exist. Skip adu-agent.service installation."
    fi

    echo "ADU components installation completed."
    echo ""
}

OS=""
VER=""
determine_distro() {
    # shellcheck disable=SC1091

    # Checking distro name and version
    if [ -r /etc/os-release ]; then
        # freedesktop.org and systemd
        OS=$(grep "^ID\s*=\s*" /etc/os-release | sed -e "s/^ID\s*=\s*//")
        VER=$(grep "^VERSION_ID\s*=\s*" /etc/os-release | sed -e "s/^VERSION_ID\s*=\s*//")
        VER=$(sed -e 's/^"//' -e 's/"$//' <<<"$VER")
    elif type lsb_release >/dev/null 2>&1; then
        # linuxbase.org
        OS=$(lsb_release -si)
        VER=$(lsb_release -sr)
    elif [ -f /etc/lsb-release ]; then
        # For some versions of Debian/Ubuntu without lsb_release command
        OS=$(grep "^DISTRIB_ID\s*=\s*" /etc/lsb-release | sed -e "s/^DISTRIB_ID\s*=\s*//")
        VER=$(grep "^DISTRIB_RELEASE\s*=\s*" /etc/lsb-release | sed -e "s/^DISTRIB_RELEASE\s*=\s*//")
    elif [ -f /etc/debian_version ]; then
        # Older Debian/Ubuntu/etc.
        OS=Debian
        VER=$(cat /etc/debian_version)
    else
        # Fall back to uname, e.g. "Linux <version>", also works for BSD, etc.
        OS=$(uname -s)
        VER=$(uname -r)
    fi
    OS="$(echo "$OS" | tr '[:upper:]' '[:lower:]')"
}

determine_distro

while [[ $1 != "" ]]; do
    case $1 in
    -c | --clean)
        build_clean=true
        ;;
    -t | --type)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "-t build type parameter is mandatory."
            $ret 1
        fi
        build_type=$1
        ;;
    -d | --build-docs)
        build_documentation=true
        ;;
    -u | --build-unit-tests)
        build_unittests=true
        ;;
    --build-packages)
        build_packages=true
        ;;
    -o | --out-dir)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "-o output directory parameter is mandatory."
            $ret 1
        fi
        output_directory=$1

        ;;
    -s | --static-analysis)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "-s static analysis tools parameter is mandatory."
            $ret 1
        fi
        if [[ $1 == "all" ]]; then
            declare -a static_analysis_tools=(clang-tidy cppcheck cpplint iwyu lwyu)
        else
            IFS=','
            read -ra static_analysis_tools <<<"$1"
            IFS=' '
        fi
        ;;
    -p | --platform-layer)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "-p platform layer parameter is mandatory."
            $ret 1
        fi
        platform_layer=$1

        ;;
    --log-lib)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--log-lib parameter is mandatory."
            $ret 1
        fi
        log_lib=$1
        ;;
    -l | --log-dir)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "-l log directory  parameter is mandatory."
            $ret 1
        fi
        adu_log_dir=$1
        ;;
    --install-prefix)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--install-prefix parameter is mandatory."
            $ret 1
        fi
        install_prefix=$1
        ;;
    --install)
        if [[ $EUID -ne 0 ]]; then
            error "Super-user required to install ADU components."
            $ret 1
        fi
        install_adu="true"
        ;;
    --cmake-path)
        shift
        cmake_dir_path=$1
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

if [[ $build_documentation == "true" ]]; then
    if ! [ -x "$(command -v doxygen)" ]; then
        error "Can't build documentation - doxygen is not installed. Try: apt install doxygen"
        $ret 1
    fi

    if ! [ -x "$(command -v dot)" ]; then
        error "Can't build documentation - dot (graphviz) is not installed. Try: apt install graphviz"
        $ret 1
    fi
fi

# Set default log dir if not specified.
if [[ $adu_log_dir == "" ]]; then
    if [[ $platform_layer == "simulator" ]]; then
        adu_log_dir=/tmp/aduc-logs
    else
        adu_log_dir=$default_log_dir
    fi
fi

runtime_dir=${output_directory}/bin
library_dir=${output_directory}/lib

# Output banner
echo ''
header "Building ADU Agent"
bullet "Clean build: $build_clean"
bullet "Documentation: $build_documentation"
bullet "Platform layer: $platform_layer"
bullet "Content handlers: $content_handlers"
bullet "Build type: $build_type"
bullet "Log directory: $adu_log_dir"
bullet "Logging library: $log_lib"
bullet "Output directory: $output_directory"
bullet "Build unit tests: $build_unittests"
bullet "Build packages: $build_packages"
if [[ ${#static_analysis_tools[@]} -eq 0 ]]; then
    bullet "Static analysis: (none)"
else
    bullet "Static analysis: " "${static_analysis_tools[@]}"
fi
echo ''

# Store options for CMAKE in an array
CMAKE_OPTIONS=(
    "-DADUC_BUILD_DOCUMENTATION:BOOL=$build_documentation"
    "-DADUC_BUILD_UNIT_TESTS:BOOL=$build_unittests"
    "-DADUC_BUILD_PACKAGES:BOOL=$build_packages"
    "-DADUC_CONTENT_HANDLERS:STRING=$content_handlers"
    "-DADUC_LOG_FOLDER:STRING=$adu_log_dir"
    "-DADUC_LOGGING_LIBRARY:STRING=$log_lib"
    "-DADUC_PLATFORM_LAYER:STRING=$platform_layer"
    "-DCMAKE_BUILD_TYPE:STRING=$build_type"
    "-DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON"
    "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY:STRING=$library_dir"
    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY:STRING=$runtime_dir"
    "-DCMAKE_INSTALL_PREFIX=$install_prefix"
)

for i in "${static_analysis_tools[@]}"; do
    case $i in
    clang-tidy)
        # http://clang.llvm.org/extra/clang-tidy/ (sudo apt install clang-tidy)

        if ! [ -x "$(command -v clang-tidy)" ]; then
            error "Can't run static analysis - clang-tidy is not installed. Try: apt install clang-tidy"
            $ret 1
        fi

        # clang-tidy requires clang to be installed so that it can find clang headers.
        if ! [ -x "$(command -v clang)" ]; then
            error "Can't run static analysis - clang is not installed. Try: apt install clang"
            $ret 1
        fi

        CMAKE_OPTIONS+=('-DCMAKE_C_CLANG_TIDY=/usr/bin/clang-tidy')
        CMAKE_OPTIONS+=('-DCMAKE_CXX_CLANG_TIDY=/usr/bin/clang-tidy')
        ;;
    cppcheck)
        # http://cppcheck.sourceforge.net/ (sudo apt install cppcheck)

        if ! [ -x "$(command -v cppcheck)" ]; then
            error "Can't run static analysis - cppcheck is not installed. Try: apt install cppcheck"
            $ret 1
        fi

        # --verbose;--check-config
        # -I/usr/local/include/azureiot
        # -I/usr/local/include -- catch
        CMAKE_OPTIONS+=('-DCMAKE_CXX_CPPCHECK=/usr/bin/cppcheck;--template=''{file}:{line}: warning: ({severity}) {message}'';--platform=unix64;--inline-suppr;--std=c++11;--enable=all;--suppress=unusedFunction;--suppress=missingIncludeSystem;--suppress=unmatchedSuppression;-I/usr/include;-I/usr/include/openssl')
        CMAKE_OPTIONS+=('-DCMAKE_C_CPPCHECK=/usr/bin/cppcheck;--template=''{file}:{line}: warning: ({severity}) {message}'';--platform=unix64;--inline-suppr;--std=c99;--enable=all;--suppress=unusedFunction;--suppress=missingIncludeSystem;--suppress=unmatchedSuppression;-I/usr/include;-I/usr/include/openssl')
        ;;
    cpplint)
        # https://github.com/cpplint/cpplint (sudo pip install --system cpplint)
        #
        # Filters being applied currently apart from the obvious --whitespace:
        # legal/copyright - All source files need copyright header which will be added if we open source ADU Agent.
        # build/include - Filtering out because CppLint forces header files to be included always with the path which seems unnecessary for ADU Agent.
        # build/c++11 - CppLint calls out certain header files like <chrono> and <future> since Chromium has conflicting implementations.

        if ! [ -x "$(command -v cpplint)" ]; then
            error "Can't run static analysis - cpplint is not installed. Try: pip install --system cpplint"
            $ret 1
        fi

        CMAKE_OPTIONS+=('-DCMAKE_CXX_CPPLINT=/usr/local/bin/cpplint;--filter=-whitespace,-legal/copyright,-build/include,-build/c++11')
        ;;
    iwyu)
        # https://github.com/include-what-you-use/include-what-you-use/blob/master/README.md

        if ! [ -x "$(command -v include-what-you-use)" ]; then
            error "Can't run static analysis - include-what-you-use is not installed. Try: apt install iwyu"
            $ret 1
        fi

        CMAKE_OPTIONS+=('-DCMAKE_CXX_INCLUDE_WHAT_YOU_USE=/usr/bin/include-what-you-use')
        ;;
    lwyu)
        # Built into cmake.
        CMAKE_OPTIONS+=('-DCMAKE_LINK_WHAT_YOU_USE=TRUE')
        ;;
    *)
        warn "Invalid static analysis tool \e[1m'$i'\e[0m. Ignoring."
        ;;
    esac
done

if [[ $build_clean == "true" ]]; then
    rm -rf "$output_directory"
    rm -rf "/tmp/adu/testdata"
fi

mkdir -p "$output_directory"
pushd "$output_directory" >/dev/null

# Generate build using cmake with options
if [[ $OS != "ubuntu" || $VER != "18.04" ]]; then
        "$cmake_dir_path"/bin/cmake -G Ninja "${CMAKE_OPTIONS[@]}" "$root_dir"
else
    cmake -G Ninja "${CMAKE_OPTIONS[@]}" "$root_dir"
fi

# Do the actual building with ninja
# Save the return code of ninja so we can $ret with that return code.
ninja
ret_val=$?

if [[ $ret_val == 0 && $build_packages == "true" ]]; then
    cpack
    ret_val=$?
fi

popd >/dev/null

if [[ $ret_val == 0 && $install_adu == "true" ]]; then
    install_adu_components
fi

$ret $ret_val
