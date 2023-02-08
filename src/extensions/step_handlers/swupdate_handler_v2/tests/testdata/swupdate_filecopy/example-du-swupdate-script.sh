#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Ensure that getopt starts from first option if ". <script.sh>" was used.
OPTIND=1

ret_val=0

# Ensure we dont end the user's terminal session if invoked from source (".").
if [[ $0 != "${BASH_SOURCE[0]}" ]]; then
    ret='return'
else
    ret='exit'
fi

# Output formatting.

# Log level: 0=debug, 1=info, 2=warning, 3=error, 4=none
log_level=1

warn() { echo -e "\033[1;33mWarning:\033[0m $*" >&2; }

error() { echo -e "\033[1;31mError:\033[0m $*" >&2; }

header() { echo -e "\e[4m\e[1m\e[1;32m$*\e[0m"; }

bullet() { echo -e "\e[1;34m*\e[0m $*"; }

warn "*************************************************"
warn "*                    WARNING                    *"
warn "*                                               *"
warn "* THIS FILE IS FOR DEMONSTRATION PURPOSES ONLY. *"
warn "* DO NOT USE THIS FOR YOUR REAL PRODUCT UPDATE! *"
warn "*                                               *"
warn "*************************************************"

# Log debug prefix - blue
log_debug_pref="\033[1;30m[D]\033[0m"

# Log info prefix - blue
log_info_pref="\033[1;34m[I]\033[0m"

# Log warning prefix - yellow
log_warn_pref="\033[1;33m[W]\033[0m"

# Log error prefix - red
log_error_pref="\033[1;31m[E]\033[0m"

#
# Files and Folders information
#
workfolder=
swu_file=
output_file=
log_file=
swu_log_file=
result_file=

#
# Following files are based on the how the Yocto Refererence Image was built.
# This value can be replaced in this script, or specify in import manifest (handlerProperties) as needed.
#
software_version_file="/etc/adu-version"
public_key_file="/adukey/public.pem"

#
# Device Update specific arguments
#
check_is_installed=
installed_criteria=
do_download_action=
do_install_action=
do_apply_action=
do_cancel_action=

restart_to_apply=
restart_agent_to_apply=

#
# Remaining aguments and parameters
#
PARAMS=

#
# Output, Logs, and Result helper functions.
#
_timestamp=

update_timestamp() {
    # See https://man7.org/linux/man-pages/man1/date.1.html
    _timestamp="$(date +'%Y/%m/%d:%H%M%S')"
}

log_debug() {
    if [ $log_level -gt 0 ]; then
        return
    fi
    log "$log_debug_pref" "$@"
}

log_info() {
    if [ $log_level -gt 1 ]; then
        return
    fi
    log "$log_info_pref" "$@"
}

log_warn() {
    if [ $log_level -gt 2 ]; then
        return
    fi
    log "$log_warn_pref" "$@"
}

log_error() {
    if [ $log_level -gt 3 ]; then
        return
    fi
    log "$log_error_pref" "$@"
}

log() {
    update_timestamp
    if [ -z "$log_file" ]; then
        echo -e "[$_timestamp]" "$@" >&1
    else
        echo "[$_timestamp]" "$@" >> "$log_file"
    fi
}

output() {
    update_timestamp
    if [ -z "$output_file" ]; then
        echo "[$_timestamp]" "$@" >&1
    else
        echo "[$_timestamp]" "$@" >> "$output_file"
    fi
}

result() {
    # NOTE: don't insert timestamp in result file.
    if [ -z "$result_file" ]; then
        echo "$@" >&1
    else
        echo "$@" > "$result_file"
    fi
}

#
# Helper function for creating extended result code that indicates
# errors from this script.
# Note: these error range (0x30101000 - 0x30101fff) a free to use.
#
# Usage: make_swupdate_handler_erc error_value result_variable
#
# e.g.
#         error_code=20
#         RESULT=0
#         make_swupdate_handler_erc RESULT $error_code
#         echo $RESULT
#
#  (RESULT is 0x30101014)
#
make_swupdate_handler_erc() {
    local base_erc=0x30101000
    local -n res=$2 # name reference
    res=$((base_erc + $1))
}

# usage: make_aduc_result_json $resultCode $extendedResultCode $resultDetails <out param>
# shellcheck disable=SC2034
function make_aduc_result_json() {
    local -n res=$4 # name reference
    res="{\"resultCode\":$1, \"extendedResultCode\":$2,\"resultDetails\":\"$3\"}"
}

#
# Usage
#
print_help() {
    echo ""
    echo "Usage: <script-file-name>.sh [options...]"
    echo ""
    echo ""
    echo "Device Update reserved argument"
    echo "==============================="
    echo ""
    echo "--action-is-installed                     Perform 'is-installed' check."
    echo "                                          Check whether the selected component [or primary device] current states"
    echo "                                          satisfies specified 'installedCriteria' data."
    echo "--installed-criteria                      Specify the Installed-Criteria string."
    echo ""
    echo "--action-download                         Perform 'download' aciton."
    echo "--action-install                          Perform 'install' action."
    echo "--action-apply                            Perform 'apply' action."
    echo "--action-cancel                           Perform 'cancel' action."
    echo ""
    echo "--restart-to-apply                        Request the host device to restart when applying update to this component."
    echo "--restart-agent-to-apply                  Request the DU Agent to restart when applying update to this component."
    echo ""
    echo "File and Folderinformation"
    echo "=========================="
    echo ""
    echo "--work-folder           A work-folder (or sandbox folder)."
    echo "--swu-file              A swu file to install."
    echo "--swu-log-file          A log file for swupdate command."
    echo "--output-file           An output file."
    echo "--log-file              A log file."
    echo "--result-file           A file containing ADUC_Result data (in JSON format)."
    echo "--software-version-file A file containing software version number."
    echo "--public-key-file       A public key file for signature validateion."
    echo "                        See InstallUpdate() function for more details."
    echo ""
    echo "--log-level <0-4>             A minimum log level. 0=debug, 1=info, 2=warning, 3=error, 4=none."
    echo "-h, --help                    Show this help message."
    echo ""
}

log "Log begin:"
output "Output begin:"

#
# Parsing arguments
#
while [[ $1 != "" ]]; do
    case $1 in

    #
    # Device Update specific arguments.
    #
    --action-download)
        shift
        do_download_action=yes
        ;;

    --action-install)
        shift
        log_info "Will runscript as 'installer' script."
        do_install_action=yes
        ;;

    --action-apply)
        shift
        do_apply_action=yes
        ;;

    --restart-to-apply)
        shift
        restart_to_apply=yes
        ;;

    --restart-agent-to-apply)
        shift
        restart_agent_to_apply=yes
        ;;

    --action-cancel)
        shift
        do_cancel_action=yes
        ;;

    --action-is-installed)
        shift
        check_is_installed=yes
        ;;

    --installed-criteria)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--installed-criteria requires an installedCriteria parameter."
            $ret 1
        fi
        installed_criteria="$1"
        shift
        ;;

    #
    # Update artifacts
    #
    --swu-file)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--swu-file parameter is mandatory."
            $ret 1
        fi
        swu_file="$1"
        echo "swu file: $swu_file"
        shift
        ;;

    --work-folder)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--work-folder parameter is mandatory."
            $ret 1
        fi
        workfolder="$1"
        echo "work folder: $workfolder"
        shift
        ;;

    #
    # Output-related arguments.
    #
    # --out-file <file_path>, --result-file <file_path>, --log-file <file_path>
    #
    --output-file)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--out-file parameter is mandatory."
            $ret 1
        fi
        output_file="$1"

        #
        #Create output file path.
        #
        # Delete existing log.
        rm -f -r "$output_file"
        # Create dir(s) recursively (include filename, well remove it in the following line...).
        mkdir -p "$output_file"
        # Delete leaf-dir (w)
        rm -f -r "$output_file"

        shift
        ;;

    --result-file)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--result-file parameter is mandatory."
            $ret 1
        fi
        result_file="$1"
        #
        #Create result file path.
        #
        # Delete existing log.
        rm -f -r "$result_file"
        # Create dir(s) recursively (include filename, well remove it in the following line...).
        mkdir -p "$result_file"
        # Delete leaf-dir (w)
        rm -f -r "$result_file"
        shift
        ;;

    --software-version-file)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--software-version-file parameter is mandatory."
            $ret 1
        fi
        software_version_file="$1"
        shift
        ;;

    --public-key-file)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--public-key-file parameter is mandatory."
            $ret 1
        fi
        public_key_file="$1"
        shift
        ;;

    --log-file)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--log-file parameter is mandatory."
            $ret 1
        fi
        log_file="$1"
        shift
        ;;

    --swu-log-file)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--swu-log-file parameter is mandatory."
            $ret 1
        fi
        swu_log_file="$1"
        shift
        ;;

    --log-level)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--log-level parameter is mandatory."
            $ret 1
        fi
        log_level=$1
        shift
        ;;

    -h | --help)
        print_help
        $ret 0
        ;;

    *) # preserve positional arguments
        PARAMS="$PARAMS $1"
        shift
        ;;
    esac
done

#
# Device Update related functions.
#

#
# A helper function that evaluates whether an update is currently installed on the target,
# based on 'installedCriteria'.
#
# Usage: is_installed $installedCriteria $softwareVersionFile <out resultCode> <out extendedResultCode> <out resultDetails>
#
# shellcheck disable=SC2034
function is_installed() {
    local -n rc=$3  # name reference for resultCode
    local -n erc=$4 # name reference for extendedResultCode
    local -n rd=$5  # name reference for resultDetails

    if ! [[ -f $software_version_file ]]; then
        rc=901
        make_swupdate_handler_erc 100 erc
        rd="Software version file doesn't exists."
    elif [[ $1 == "" ]]; then
        rc=0
        make_swupdate_handler_erc 101 erc
        rd="Installed criteria is empty."
    else
        {
            grep "^$1$" "$software_version_file"
        }

        grep_res=$?

        if [[ $grep_res -eq "0" ]]; then
            rc=900
        else
            rc=901
        fi
        erc=0
        rd=""
    fi
}

#
# Example implementation of 'IsInstalled' function, for the reference Yocto Image.
#
# Design Goal:
#   Determine whether the specified 'installedCriteria' (parameter $1) is met.
#
#   'installedCriteria' is a version number of the image (generated by DU Yocto internal build pipeline).
#   This version is saved in /etc/adu-version file.
#
# Expected resultCode:
#     ADUC_Result_Failure = 0,
#     ADUC_Result_IsInstalled_Installed = 900,     /**< Succeeded and content is installed. */
#     ADUC_Result_IsInstalled_NotInstalled = 901,  /**< Succeeded and content is not installed */
#
CheckIsInstalledState() {
    log_info "CheckIsInstalledState (\"$1\"), adu-version path:\"$software_version_file\""

    local local_rc=2
    local local_erc=3
    local local_rd="4"
    local aduc_result=""

    is_installed "$1" "$software_version_file" local_rc local_erc local_rd

    log_info "is_installed return: $local_rc $local_erc $local_rd"

    make_aduc_result_json "$local_rc" "$local_erc" "$local_rd" aduc_result

    # Show output.
    output "Result:" "$aduc_result"

    # Write ADUC_Result to result file.
    result "$aduc_result"
}

#
# Download any additional files required for the update.
#
DownloadUpdateArtifacts() {
    log_info "DownloadUpdateArtifacts called"

    # Note: for this example update, there's no other files to download.
    # Return ADUC_Result_Download_Success (500)
    resultCode=500
    extendedResultCode=0
    resultDetails=""
    ret_val=0

    # Prepare ADUC_Result json.
    aduc_result="{\"resultCode\":$resultCode, \"extendedResultCode\":$extendedResultCode,\"resultDetails\":\"$resultDetails\"}"

    # Show output.
    output "Result:" "$aduc_result"

    # Write ADUC_Result to result file.
    result "$aduc_result"

    $ret $ret_val
}

#
# InstallUpdate:
# Copies a 'firmware.json' to component's folder (properties.path).
#
InstallUpdate() {
    resultCode=0
    extendedResultCode=0
    resultDetails=""
    ret_val=0

    #
    # Note: we could simulate 'component off-line' scenario here.
    #

    # Check whether the component is already installed the specified update...
    is_installed "$installed_criteria" "$software_version_file" resultCode extendedResultCode resultDetails

    is_installed_ret=$?

    if [[ $is_installed_ret -ne 0 ]]; then
        # is_installed functin failed to execute.
        resultCode=0
        make_swupdate_handler_erc "$is_installed_ret" extendedResultCode
        resultDetails="Internal error in 'is_installed' function."
    elif [[ $resultCode == 0 ]]; then
        # Failed to determine whether the update has been installed or not.
        # Return current ADUC_Result
        echo "Failed to determine wehther the update has been installed or note."
    elif [[ $resultCode -eq 901 ]]; then
        # Not installed.

        # install an update.
        echo "Installing update." >> "${swu_log_file}"
        if [[ -f $swu_file ]]; then
            # Call swupdate with the image file
            if [[ ${public_key_file} != "" ]]; then
                swupdate -v -i "${swu_file}" -l 4 &>> "${swu_log_file}"
            else
                swupdate -v -i "${swu_file}" -k "${public_key_file}" -l 4 &>> "${swu_log_file}"
            fi
            ret_val=$?

            if [[ $ret_val -eq 0 ]]; then
                resultCode=600
                extendedResultCode=0
                resultDetails=""
            else
                resultCode=0
                make_swupdate_handler_erc "$ret_val" extendedResultCode
                resultDetails="SWUpdate command failed."
            fi
        else
            echo "Image file ${swu_file} was not found." >> "${swu_log_file}"
            resultCode=0
            # ADUC_ERC_SWUPDATE_HANDLER_INSTALL_FAILURE_SWU_FILE_NOT_FOUND (0x202)
            extendedResultCode=0x30100202
            resultDetails="Image file ${swu_file} was not found."
        fi
        ret_val=0
    else
        # Installed.
        log_info "It appears that this component already installed the specified update."
        resultCode=603
        extendedResultCode=0
        resultDetails="Already installed."
        ret_val=0
    fi

    # Prepare ADUC_Result json.
    make_aduc_result_json "$resultCode" "$extendedResultCode" "$resultDetails" aduc_result

    # Show output.
    output "Result:" "$aduc_result"

    # Write ADUC_Result to result file.
    result "$aduc_result"

    $ret $ret_val
}

#
# Perform any steps to finalize the update.
#
ApplyUpdate() {
    log_info "ApplyUpdate called (restart_to_apply:$restart_to_apply, restart_agent_to_apply:$restart_agent_to_apply)"

    # Note: for this example update, 'Apply' always succeeded.
    resultCode=700
    extendedResultCode=0
    resultDetails=""
    ret_val=0

    # Prepare ADUC_Result json.
    aduc_result="{\"resultCode\":$resultCode, \"extendedResultCode\":$extendedResultCode,\"resultDetails\":\"$resultDetails\"}"

    # Show output.
    output "Result:" "$aduc_result"

    # Write ADUC_Result to result file.
    result "$aduc_result"

    $ret $ret_val
}

#
# Cancel current update.
#
# For this example, we cannot roll back any changes.
# So, will simply return ADUC_Result_Cancel_UnableToCancel (801)
#
#
CancelUpdate() {
    log_info "CancelUpdate called"

    # Note: for this example update, there's no other files to download.
    # Return ADUC_Result_Download_Success (500)
    resultCode=801
    extendedResultCode=0
    resultDetails=""
    ret_val=0

    # Prepare ADUC_Result json.
    make_aduc_result_json "$resultCode" "$extendedResultCode" "$resultDetails" aduc_result

    # Show output.
    output "Result:" "$aduc_result"

    # Write ADUC_Result to result file.
    result "$aduc_result"

    $ret $ret_val
}

#
# Main
#

# Ensure that swu_log_file is avalid
if [[ $swu_log_file == "" ]]; then
    swu_log_file="$log_file.swupdate.log"
fi

log_info "Will save SWUpdate logs to ${swu_log_file}"

if [ -n "$check_is_installed" ]; then
    CheckIsInstalledState "$installed_criteria"
    exit $ret_val
fi

if [ -n "$do_download_action" ]; then
    DownloadUpdateArtifacts
    exit $ret_val
fi

if [ -n "$do_install_action" ]; then
    InstallUpdate
    exit $ret_val
fi

if [ -n "$do_apply_action" ]; then
    ApplyUpdate
    exit $ret_val
fi

if [ -n "$do_cancel_action" ]; then
    CancelUpdate
    exit $ret_val
fi

$ret $ret_val
