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
output_file=/adu/logs/swpupdate.output
log_file=/adu/logs/swupdate.log
swupdate_log_file=/adu/logs/swupdate_log_file
result_file=/adu/logs/swupdate.result.json

#
# Following files are based on the how the Yocto Refererence Image was built.
# This value can be replaced in this script, or specify in import manifest (handlerProperties) as needed.
#
software_version_file="/etc/adu-version"
public_key_file="/adukey/public.pem"

#
# For install task, the --image-file <image_file_name> option must be specified.
#
image_file=""

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

# SWUpdate doesn't support everything necessary for the dual-copy or A/B update strategy.
# Here we figure out the current OS partition and then set some environment variables
# that we use to tell swupdate which partition to target.
rootfs_dev=$(mount | grep "on / type" | cut -d':' -f 2 | cut -d' ' -f 1)
if [[ $rootfs_dev == '/dev/mmcblk0p2' ]]; then
    selection="stable,copy2"
    current_partition=2
    update_partition=3
else
    selection="stable,copy1"
    current_partition=3
    update_partition=2
fi

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
    if [ -z $log_file ]; then
        echo -e "[$_timestamp]" "$@" >&1
    else
        echo "[$_timestamp]" "$@" >> $log_file
    fi
}

output() {
    update_timestamp
    if [ -z $output_file ]; then
        echo "[$_timestamp]" "$@" >&1
    else
        echo "[$_timestamp]" "$@" >> "$output_file"
    fi
}

result() {
    # NOTE: don't insert timestamp in result file.
    if [ -z $result_file ]; then
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
make_aduc_result_json() {
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
    echo "File and Folder information"
    echo "==========================="
    echo ""
    echo "--workfolder              A work-folder (or sandbox folder)."
    echo "--swu-file, --image-file  An image file (.swu) file to install."
    echo "--output-file             An output file."
    echo "--log-file                A log file."
    echo "--swupdate-log-file       A file contains output from swupdate tool."
    echo "--result-file             A file contain ADUC_Result data (in JSON format)."
    echo "--software-version-file   A file contain image version number."
    echo "--public-key-file         A public key file for signature validateion."
    echo "                          See InstallUpdate() function for more details."
    echo ""
    echo "--log-level <0-4>         A minimum log level. 0=debug, 1=info, 2=warning, 3=error, 4=none."
    echo "-h, --help                Show this help message."
    echo ""
    echo "Example:"
    echo ""
    echo "Scenario: is-installed check"
    echo "========================================"
    echo "    <script> --log-level 0 --action-is-installed --intalled-criteria 1.0"
    echo ""
    echo "Scenario: perform install action"
    echo "================================"
    echo "    <script> --log-level 0 --action-install --intalled-criteria 1.0 --swu-file example-device-update.swu --workfolder <sandbox-folder>"
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

    --action_cancel)
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
        image_file="$1"
        echo "swu file (image_file): $image_file"
        shift
        ;;

    --workfolder)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--workfolder parameter is mandatory."
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

    --image-file)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--image-file parameter is mandatory."
            $ret 1
        fi
        image_file="$1"
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

    --swupdate-log-file)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--swupdate-log-file parameter is mandatory."
            $ret 1
        fi
        swupdate_log_file="$1"
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
# Usage: is_installed $installedCriteria $imageVersionFile <out resultCode> <out extendedResultCode> <out resultDetails>
#
# shellcheck disable=SC2034
function is_installed() {
    local -n rc=$3  # name reference for resultCode
    local -n erc=$4 # name reference for extendedResultCode
    local -n rd=$5  # name reference for resultDetails

    if ! [[ -f $2 ]]; then
        rc=0
        make_swupdate_handler_erc 100 erc
        rd="Image version file name is empty."
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
#   This version is saved in $software_version_file.
#
# Expected resultCode:
#     ADUC_Result_Failure = 0,
#     ADUC_Result_IsInstalled_Installed = 900,     /**< Succeeded and content is installed. */
#     ADUC_Result_IsInstalled_NotInstalled = 901,  /**< Succeeded and content is not installed */
#
CheckIsInstalledState() {
    log_info "IsInstalledTask(\"$1\"), adu-version path:\"$software_version_file\""

    local local_rc=2
    local local_erc=3
    local local_rd="4"
    local aduc_result=""

    # Evaluates installedCriteria string.
    is_installed "$1" "$software_version_file" local_rc local_erc local_rd

    make_aduc_result_json "$local_rc" "$local_erc" "$local_rd" aduc_result

    # Show output.
    output "Result:" "$aduc_result"

    # Write ADUC_Result to result file.
    result "$aduc_result"
}

#
# Example implementation of 'DownloadUpdateArtifacts' function, for the reference Yocto Image.
#
# This fuction is no-op since no additional files are required for this update.
#
DownloadUpdateArtifacts() {
    log_info "DownloadUpdateArtifacts called"

    # Return ADUC_Result_Download_Success (500), no extended result code, no result details.
    make_aduc_result_json 500 0 "" aduc_result

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
    log_info "InstallUpdate called"

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
        echo "Installing update." >> "${log_file}"
        if [[ -f $image_file ]]; then

            # Note: Swupdate can use a public key to validate the signature of an image.
            #
            # Here is how we generated the private key for signing the image
            # and how we generated that public key file used to validate the image signature.
            #
            # Generated RSA private key with password using command:
            # openssl genrsa -aes256 -passout file:priv.pass -out priv.pem
            #
            # Generated RSA public key from private key using command:
            # openssl rsa -in ${WORKDIR}/priv.pem -out ${WORKDIR}/public.pem -outform PEM -pubout

            if [[ ${public_key_file} -eq "" ]]; then
                # Call swupdate with the image file and no signature validations
                swupdate -v -i "${image_file}" -e ${selection} &>> "${swupdate_log_file}"
            else
                # Call swupdate with the image file and the public key for signature validation
                swupdate -v -i "${image_file}" -k "${public_key_file}" -e ${selection} &>> "${swupdate_log_file}"
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
            echo "Image file ${image_file} was not found." >> "${log_file}"
            resultCode=0
            # ADUC_ERC_SWUPDATE_HANDLER_INSTALL_FAILURE_IMAGE_FILE_NOT_FOUND (0x202)
            extendedResultCode=0x30100202
            resultDetails="Image file ${image_file} was not found."
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
# Performs steps to finalize the update.
#
ApplyUpdate() {
    log_info "Applying update..."

    ret_val=0
    resultCode=0
    extendedResultCode=0
    resultDetails=""

    echo "Cancelling update." >> "${log_file}"

    # Set the bootloader environment variable
    # to tell the bootloader to boot into the new partition.
    #
    # 'rpipart' variable is specific to our boot.scr script.
    fw_setenv rpipart $update_partition
    ret_val=$?

    if [[ $ret_val -eq 0 ]]; then
        if [ "$restart_to_apply" == "yes" ]; then
            # ADUC_Result_Apply_RequiredImmediateReboot = 705
            resultCode=705
        elif [ "$restart_agent_to_apply" == "yes" ]; then
            # ADUC_Result_Apply_RequiredImmediateAgentRestart = 707
            resultCode=707
        else
            # ADUC_Result_Apply_Success = 700
            resultCode=700
        fi
        extendedResultCode=0
        resultDetails=""
    else
        resultCode=0
        make_swupdate_handler_erc "$ret_val" extendedResultCode
        resultDetails="Cannot set rpipart value to $update_partition"
        log_error "$resultDetails"
    fi

    # Prepare ADUC_Result json.
    aduc_result=""
    make_aduc_result_json "$resultCode" "$extendedResultCode" "$resultDetails" aduc_result

    # Show output.
    output "Result:" "$aduc_result"

    # Write ADUC_Result to result file.
    result "$aduc_result"

    $ret $ret_val
}

#
# Cancel current update.
#
# Set the bootloader environment variable to tell the bootloader to boot into the current partition
# instead of the one that was updated. Note: rpipart variable is specific to our boot.scr script.
#
CancelUpdate() {
    log_info "CancelUpdate called"

    # Set the bootloader environment variable
    # to tell the bootloader to boot into the current partition.
    # rpipart variable is specific to our boot.scr script.
    echo "Cancelling update." >> "${log_file}"
    resultCode=0
    extendedResultCode=0
    resultDetails=""
    ret_val=

    echo "Revert update." >> "${log_file}"
    fw_setenv rpipart $current_partition
    ret_val=$?

    if [[ $ret_val -eq 0 ]]; then
        resultCode=800
    else
        resultCode=801
        make_swupdate_handler_erc "$ret_val" extendedResultCode
        resultDetails="Failed to set rpipart to ${update_partition}"
        ret_val=0
    fi

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
