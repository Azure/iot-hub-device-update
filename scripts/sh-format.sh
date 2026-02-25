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

validate=false

# use readline -e to resolve symlink
shellcheck_bin=$(readlink -e '/tmp/deviceupdate-shellcheck')
shfmt_bin='/tmp/shfmt'

is_arm32=false
is_amd64=false

# Attempts to determine the machine cpu architecture by pattern matching the
# output of uname -m
# Returns 0 on success and sets the following variables as side-effect:
# is_arm32, is_amd64
determine_machine_architecture() {
    local arch=''
    arch="$(uname -m)"
    local ret_val=$?
    if [[ $ret_val != 0 ]]; then
        error "Failed to get cpu architecture."
        return 1
    else
        if [[ $arch == armv7* || $arch == 'arm' ]]; then
            is_arm32=true
        elif [[ $arch == 'x86_64' || $arch == 'amd64' ]]; then
            is_amd64=true
        else
            error "Machine architecture '$arch' is not supported."
            return 1
        fi
    fi
}

warn() { echo -e "\033[1;33mWarning:\033[0m $*" >&2; }

error() { echo -e "\033[1;31mError:\033[0m $*" >&2; }

print_help() {
    echo "Usage: sh-format.sh [options...]"
    echo "-v, --validate        Validates shell scripts as well as format."
    echo "-h, --help            Show this help message."
}

while [[ $1 != "" ]]; do
    case $1 in
    -v | --validate)
        validate=true
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

if [[ ! -e $shfmt_bin || ! -x $shfmt_bin ]]; then
    # Do not use snap because on ubuntu 20.04 it is a version that has trouble with finding .editorconfig but reported as permissions issue
    warn 'shfmt is not installed. Trying to install...'

    if ! determine_machine_architecture; then
        $ret 1
    fi

    SHFMT_GIT_BASE_URL="https://github.com/patrickvane/shfmt/releases/download/master"

    # shfmt releases _arm is arm32 as per readelf -h -A and there is no arm64
    arch_suffix=''
    if [[ $is_arm32 == "true" ]]; then
        arch_suffix='arm'
    elif [[ $is_amd64 == "true" ]]; then
        arch_suffix='amd64'
    else
        warn "not arm32 or amd64. Try: wget ${SHFMT_GIT_BASE_URL}/shfmt_linux_{ARCH}"
        warn '    chmod 755 shfmt_linux_{ARCH} && ln -s /path/to/shfmt_linux_{ARCH} /somewhere/in/your/PATH/shfmt'
        warn '  OR last resort Try: snap install shfmt'
        $ret 1
    fi

    if [[ $arch_suffix != "" ]]; then
        release_asset="shfmt_linux_${arch_suffix}"
        release_asset_url="${SHFMT_GIT_BASE_URL}/${release_asset}"

        wget -P /tmp "$release_asset_url" || $ret 1
        chmod u+rwx,go+rx "/tmp/${release_asset}" || $ret 1
        ln -sf "/tmp/${release_asset}" "$shfmt_bin" || $ret 1
    fi
fi

if ! [ -x "$(command -v git)" ]; then
    error 'git is not installed. Try: apt install git'
    $ret 1
fi

GITROOT="$(git rev-parse --show-toplevel 2> /dev/null)"
if [ -z "$GITROOT" ]; then
    error 'Unable to determine git root.'
    $ret 1
fi

pushd "$GITROOT" > /dev/null || $ret
# diff-filter=d will exclude deleted files. Use --cached to only focus on
# staged files to avoid bypass of pre-commit hook using unstaged working set.
IFS=$'\n'
shell_files=$(git diff --diff-filter=d --relative --name-only --cached HEAD -- "*.sh" "*.bash" "*.mksh")
shfmt_failed=false
for FILE in $shell_files; do
    echo "Formatting $FILE"

    # Use git cat-file blob to get the staged contents only for a dry-run to test if a format would occur.
    # -w cannot be used when piping contents into shfmt.
    resolved_shfmt_bin=$(readlink -e $shfmt_bin)
    if ! git cat-file blob :"$FILE" 2>&1 | $resolved_shfmt_bin -d -s -i 4 -bn -sr; then
        shfmt_failed=true

        # use -w now to actually in-place write the reformatting.
        $resolved_shfmt_bin -d -s -w -i 4 -bn -sr "$FILE"
    fi
done

if [[ $shfmt_failed == "true" ]]; then
    error 'shfmt failed or reformatted a file. Fix failures and/or git add the unstaged changes before commit'
    $ret 1
fi

if [[ $validate == "true" ]]; then
    if [[ ! -x $shellcheck_bin ]]; then
        error "'$shellcheck_bin' is not installed or executable. Try: ./scripts/install-deps.sh --install-shellcheck"
        $ret 1
    fi

    shellcheck_validation_failed=false
    for FILE in $shell_files; do
        echo "Checking $FILE"
        if ! $shellcheck_bin --shell=bash --severity=style "$FILE"; then
            shellcheck_validation_failed=true
            # continue on to next file
        fi
    done

    if [[ $shellcheck_validation_failed == "true" ]]; then
        error "shellcheck failed. Fix the issue(s) and try again."
        $ret 1
    fi
fi
IFS=' '
popd > /dev/null || $ret

$ret 0
