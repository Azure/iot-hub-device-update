#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

OPTIND=1  # Ensure that getopt starts from first option if ". <script.sh>" was used.
ret_val=0 # Global script exit code

# Ensure we don't end the user's terminal session if invoked from source (".").
ret=$([[ $0 != "${BASH_SOURCE[0]}" ]] && echo 'return' || echo 'exit')

# Output formatting. Log level: 0=debug, 1=info, 2=warning, 3=error, 4=none
log_level=1
log_debug_pref="\033[1;30m[D]\033[0m"  # Log debug prefix   - blue
log_info_pref="\033[1;34m[I]\033[0m"   # Log info prefix    - blue
log_warn_pref="\033[1;33m[W]\033[0m"   # Log warning prefix - yellow
log_error_pref="\033[1;31m[E]\033[0m"  # Log error prefix   - red

info() { echo -e "\033[1;34m#\033[0m $*"; }
warn() { echo -e "\033[1;33mWarning:\033[0m $*" >&2; }
error() { echo -e "\033[1;31mError:\033[0m $*" >&2; }
header() { echo -e "\e[4m\e[1m\e[1;32m$*\e[0m"; }
bullet() { echo -e "\e[1;34m*\e[0m $*"; }
bold() { echo -e "\e[1m\e[97m$*\e[0m"; }

bold "##############################################################################"
bold ""
bold " DISCLAIMER:"
bold ""
bold " THIS EXAMPLE SNAP PACKAGE UPDATE SCRIPT IS PROVIDED "AS IS", WITHOUT WARRANTY"
bold " OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES "
bold " OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. "
bold ""
bold " IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, "
bold " DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR "
bold " OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE"
bold " OR OTHER DEALINGS IN THE SOFTWARE. "
bold ""
bold " IT IS NOT ADVISIABLE TO USE THIS FILE IN A PRODUCTION ENVIRONMENT WITHOUT A"
bold " THROUGH REVIEW."
bold ""
bold "##############################################################################"
bold ""

info "INPUT PARAMETERS: $@"

#
# Files and Folders information
#
work_folder=
output_file=/var/log/adu/snap-update.output
log_file=/var/log/adu/snap-update.log
result_file=/var/log/adu/snap-update-result.json

DEVICEUPDATE_ACTION=
DEVICEUPDATE_WORKFOLDER=
DEVICEUPDATE_LOGFILE=
DEVICEUPDATE_RESULTFILE=
DEVICEUPDATE_OUTPUTFILE=

#
# Device Update specific arguments
#
check_is_installed=
installed_criteria=
do_download_action=
do_install_action=
do_apply_action=
do_cancel_action=

max_install_wait_time=$((60*60*4)) # Default installation wait time is 4 hours.

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

# Logging functions
log_debug() { [ $log_level -gt 0 ] && return; log "$log_debug_pref" "$@"; }
log_info() { [ $log_level -gt 1 ] && return; log "$log_info_pref" "$@"; }
log_warn() { [ $log_level -gt 2 ] && return; log "$log_warn_pref" "$@"; }
log_error() { [ $log_level -gt 3 ] && return; log "$log_error_pref" "$@"; }

# Write to log file
log() { update_timestamp; [ -z $log_file ] && echo -e "[$_timestamp]" "$@" >&1 || echo "[$_timestamp]" "$@" >> "$log_file"; }

# Write to output file
output() { update_timestamp; [ -z $output_file ] && echo "[$_timestamp]" "$@" >&1 || echo "[$_timestamp]" "$@" >> "$output_file"; }

# Write to result file
result() { [ -z $result_file ] && echo "$@" >&1 || echo "$@" > "$result_file"; }

#
# Helper function for creating extended result code that indicates
# errors from this script.
# Note: these error range (0x30501000 - 0x30501fff) are free to use.
#
# Usage: make_script_handler_erc error_value result_variable
#
# e.g.
#         error_code=20
#         RESULT=0
#         make_script_handler_erc $error_code RESULT
#         echo $RESULT
#
#  (RESULT is 0x30501014)
#
make_script_handler_erc() {
    local base_erc=0x30501000
    local -n res=$2 # name reference
    res=$((base_erc + $1))
}

# usage: make_aduc_result_json $resultCode $extendedResultCode $resultDetails <out param>
# shellcheck disable=SC2034
make_aduc_result_json() {
    local -n res=$4 # name reference
    res="{\"resultCode\":$1, \"extendedResultCode\":$2,\"resultDetails\":\"$3\"}"
}

# prepare_and_save_aduc_result - A function to wrap up an operation and generate an ADUC_Result JSON string.

# Usage:
#   end_action <result_code> <extended_result_code> <result_details> <ret_val>

# Arguments:
#   result_code - The result code of the operation.
#   extended_result_code - The extended result code of the operation.
#   result_details - The details of the operation result.
#   ret_val - The return value of the function.

# Output:
#   This function outputs the ADUC_Result JSON string to the console and writes it to a result file.

# Returns:
#   None.

#
# Print usage message.
#
print_help() {
    echo ""
    echo "Usage: <script-file-name>.sh [options...]"
    echo ""
    echo ""
    echo "Device Update reserved argument"
    echo "==============================="
    echo ""
    echo "--action <ACTION_NAME>            Perform the specified action."
    echo "                                  Where ACTION_NAME is is-installed, download, install, apply, cancel, backup and restore."
    echo ""
    echo "--action-is-installed             Perform 'is-installed' check."
    echo "                                  Check whether the selected component [or primary device] current states"
    echo "                                  satisfies specified 'installedCriteria' data."
    echo "--installed-criteria              Specify the Installed-Criteria string."
    echo ""
    echo "--max-install-wait-time           The timeout period for snap installation is the maximum wait time before giving up, in seconds."
    echo "                                  Default wait time is 4 hours."
    echo ""
    echo "--action-download                 Perform 'download' aciton."
    echo "--action-install                  Perform 'install' action."
    echo "--action-apply                    Perform 'apply' action."
    echo "--action-cancel                   Perform 'cancel' action."
    echo ""
    echo "File and Folder information"
    echo "==========================="
    echo ""
    echo "--work-folder             A work-folder (or sandbox folder)."
    echo "--output-file             An output file."
    echo "--log-file                A log file."
    echo "--result-file             A file contain ADUC_Result data (in JSON format)."
    echo ""
    echo "--log-level <0-4>         A minimum log level. 0=debug, 1=info, 2=warning, 3=error, 4=none."
    echo "-h, --help                Show this help message."
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
    --action)
        DEVICEUPDATE_ACTION=$2
        shift 2
        ;;

    #
    # Script Handler V1 backward compatibility
    #
    --action-download)
        shift
        DEVICEUPDATE_ACTION=download
        ;;

    --action-install)
        shift
        DEVICEUPDATE_ACTION=install
        ;;

    --action-apply)
        shift
        DEVICEUPDATE_ACTION=apply
        ;;

    --action_cancel)
        shift
        DEVICEUPDATE_ACTION=cancel
        ;;

    --action-is-installed)
        shift
        DEVICEUPDATE_ACTION=is-installed
        ;;

    --installed-criteria)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            log_error "--installed-criteria requires an installedCriteria parameter."
            $ret 1
        fi
        installed_criteria="$1"
        shift
        ;;

    --max-install-wait-time)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            log_error "--max-install-wait-time requires a wait time (in seconds) parameter."
            $ret 1
        fi
        max_install_wait_time=$1
        shift
        ;;

    #
    # Update artifacts
    #
    --work-folder)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            log_error "--work-folder parameter is mandatory."
            $ret 1
        fi
        work_folder="$1"
        echo "work folder: $work_folder"
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
            log_error "--out-file parameter is mandatory."
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
            log_error "--result-file parameter is mandatory."
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

    --log-file)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            log_error "--log-file parameter is mandatory."
            $ret 1
        fi
        log_file="$1"
        shift
        ;;

    --log-level)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            log_error "--log-level parameter is mandatory."
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

# Support for Script Handler V2
case $DEVICEUPDATE_ACTION in
    is-installed)
    check_is_installed=yes
    ;;

    download)
    do_download_action=yes
    ;;

    install)
    do_install_action=yes
    ;;

    apply)
    do_apply_action=yes
    ;;

    backup)
    do_backup_action=yes
    ;;

    restore)
    do_restore_action=yes
    ;;

    cancel)
    do_cancel_action=yes
    ;;
esac

#
# Device Update related functions.
#

#
# A helper function that evaluates whether a specified snap is currently installed on the target,
# based on 'installedCriteria'.
#
# Usage: is_installed $snap_name $snap_version $snap_channel <out result_code> <out extended_result_code> <out result_details>
#
# shellcheck disable=SC2034
function is_installed {
    local snap_name=$1
    local snap_version=$2
    local snap_channel=$3
    local -n result_code=$4  # name reference for result_code
    local -n extended_result_code=$5 # name reference for extended_result_code
    local -n result_details=$6  # name reference for result_details

    # Use Snapd RESTful API to check if package is installed
    response=$(curl $debug --unix-socket /run/snapd.socket "http://localhost/v2/snaps/$snap_name?select=version,channel")
    status_code=$(echo $response | jq -r ' . | .["status-code"]' )

    info "RESPONSE: $response"
    info "STATUS CODE: $status_code"

    # Determine if package is installed or not
    if [ "$status_code" == "200" ]; then
        snap_version_installed=$(echo $response | jq -r '.result.version')
        snap_channel_installed=$(echo $response | jq -r '.result.channel')
        info "FOUND: $snap_name, ver:\"$snap_version_installed\", channel:\"$snap_channel_installed\""
        info "EXPECTED: $snap_name, ver:\"$snap_version\", channel:\"$snap_channel\""
        if ([ "$snap_version" == "" ] || [ "$snap_version" == "$snap_version_installed" ]) && ( [ "$snap_channel" == "" ] || [ "$snap_channel" == "$snap_channel_installed" ] ); then
            result_code=900
            result_details=""
        else
            result_code=901
            result_details="Package $snap_name, version: $snap_version, channel: $snap_channel is not installed"
        fi
    elif [ "$status_code" == "404" ]; then
        result_code=901
        result_details="Package is not installed (status code 404)"
    else
        result_code=901
        result_details="Error: unexpected http status code $status_code"
        extended_result_code=$status_code
    fi
}

# This function determines whether the snap package, as specified 'installedCriteria' (parameter $1) is currrnetly installed on the device.
#
#   'installedCriteria' is a comma-delimited string containing the snap name, version, and channel.
#    the version and channel are optional.
#
#    Example installed criteria string:
#         'deviceupdate-agent,0.1,stable'
#         'deviceupdate-agent,,stable'
#         'deviceupdate-agent,0.1,'
#
# Expected resultCode:
#     ADUC_Result_Failure = 0,
#     ADUC_Result_IsInstalled_Installed = 900,     /**< Succeeded and content is installed. */
#     ADUC_Result_IsInstalled_NotInstalled = 901,  /**< Succeeded and content is not installed */
#
CheckIsInstalledState() {
    log_info "IsInstalled Task (\"$1\")"

    local installed_criteria=$1
    local local_rc=0
    local local_erc=0
    local local_rd="4"
    local aduc_result=""

    # Spit installed criteria string into 3 parts.
    IFS=',' read -ra criteria <<< "$installed_criteria"

    snap_name="${criteria[0]}"
    snap_version="${criteria[1]}"
    snap_channel="${criteria[2]}"

    # Evaluates installedCriteria string.
    is_installed "$snap_name" "$snap_version" "$snap_channel" local_rc local_erc local_rd

    prepare_and_save_aduc_result "$local_rc" "$local_erc" "$local_rd"
    ret_val=0
}

#
# No additional files to download, returns 500.
#
DownloadUpdateArtifacts() {
    log_info "Download action is not required. Returning ADUC_Result_Download_Success (500)."

    # Return ADUC_Result_Download_Success (500), no extended result code, no result details.
    prepare_and_save_aduc_result 500 0 ""
    $ret 0
}

# prepare_and_save_aduc_result - A function to wrap up an operation and generate an ADUC_Result JSON string.

# Usage:
#   end_action <result_code> <extended_result_code> <result_details> <ret_val>

# Arguments:
#   result_code - The result code of the operation.
#   extended_result_code - The extended result code of the operation.
#   result_details - The details of the operation result.
#   ret_val - The return value of the function.

# Output:
#   This function outputs the ADUC_Result JSON string to the console and writes it to a result file.

# Returns:
#   None.

# Example usage:
#   prepare_and_save_aduc_result 0 0 "Operation succeeded"
#   This will wrap up the operation with result code 0, extended result code 0, and result details "Operation succeeded".
prepare_and_save_aduc_result() {
    result_code=$1
    extended_result_code=$2
    result_details=$3
    aduc_result=""

    # Prepare ADUC_Result json.
    make_aduc_result_json "$result_code" "$extended_result_code" "$result_details" aduc_result

    # Write to output file.
    output "Result:" "$aduc_result"

    # Write ADUC_Result to result file.
    result "$aduc_result"
}

#
# To install a specific snap package, make a request to the snapd Restful API.
#
InstallUpdate() {
    local installed_criteria=$1
    log_info "Begin snap installation task (installed_criteria:'$1')"

    resultCode=0
    extendedResultCode=0
    resultDetails=""
    ret_val=0

    # Spit installed criteria string into 3 parts.
    IFS=',' read -ra criteria <<< "$installed_criteria"

    snap_name="${criteria[0]}"
    snap_version="${criteria[1]}"
    snap_channel="${criteria[2]}"

    snapd_api_data="\"action\": \"install\""

    if [ -z "$snap_name" ]; then
        resultDetails="Error: Snap name not specified. Check installed criteria string. (\"$installed_criteria\")"
        log_error "$resultDetails"
        resultCode=0
        # ADUC_ERC_SCRIPT_HANDLER_MISSING_INSTALLED_CRITERIA (0x30500002)
        extendedResultCode=$((0x30500002))
        prepare_and_save_aduc_result $resultCode $extendedResultCode $resultDetails
        $ret $ret_val
    fi

    # Build POST data
    if [ -n "$snap_version" ]; then
        snapd_api_data="$snapd_api_data, \"revision\": \"$snap_version\""
    fi

    if [ -n "$snap_channel" ]; then
        snapd_api_data="$snapd_api_data, \"channel\": \"$snap_channel\""
    fi


    snapd_api_data="{$snapd_api_data}"
    info "SNAP install params: $snapd_api_data"

    # Check whether the specified snap is already installed on the device.
    is_installed "$snap_name" "$snap_version" "$snap_channel" resultCode extendedResultCode resultDetails

    is_installed_ret=$?

    if [[ $is_installed_ret -ne 0 ]]; then
        # is_installed function failed to execute.
        resultDetails="Internal error in 'is_installed' function."
        log_error "$resultDetails"
        resultCode=0
        make_script_handler_erc "$is_installed_ret" extendedResultCode
    elif [[ $resultCode == 0 ]]; then
        # Return current ADUC_Result
        resultDetails="Failed to determine wehther the specified snap has been installed or not."
        log_error "$resultDetails"
        resultCode=0
        make_script_handler_erc "$is_installed_ret" extendedResultCode
    elif [[ $resultCode -eq 901 ]]; then
        # The snap has not been installed. Let's install it now.
        header "POST snap install request for ('$installed_criteria')"
        log_info "Installing snap '$installed_criteria'."

        # Make a POST request to the snapd daemon to install the snap
        request_command="curl -sX POST --unix-socket /run/snapd.socket \"http://localhost/v2/snaps/$snap_name\" -H \"Content-Type: application/json\" -d \"$snapd_api_data\" "
        info "Request: $request_command"

        # NOTE: For somereason, calling response_body=$( $request_command ) here result in 404.
        response_body=$( curl -sX POST --unix-socket /run/snapd.socket "http://localhost/v2/snaps/$snap_name" -H "Content-Type: application/json" -d "$snapd_api_data" )

        info "Response: $response_body"

        # Sanitize the response body, since the response body could be malformed:
        #
        # e.g.
        #
        #  400 Bad Request: malformed Host header{"type":"error","status-code":404,"status":"Not Found","result":{"message":"not found"}}
        #
        # We'll remove text befor the first curly brace.
        brace_pos=$(expr index "$response_body" "{")
        if [ $brace_pos > 1 ]; then
            response_body=${response_body:$brace_pos-1}
            info "Initial installation response: $response_body"
        fi

        # Check the response status code and display appropriate messages
        status_code=$(echo $response_body | jq -r ' . | .["status-code"]' )
        info "Response Status Code: $status_code"
        if [ -z "$status_code" ]; then
            if echo "$response_body" | grep -q '"kind":"snap-already-installed"'; then
                info "Snap is already installed:"
                snap_info=$(snap info "$snap_name")
                info "$snap_info"

                # Installed.
                log_info "It appears that the specified snap is already installed."
                resultCode=603
                extendedResultCode=0
                resultDetails="Already installed."
                ret_val=0
            elif echo "$response_body" | grep -q '"kind":"snap-not-found"'; then
                resultDetails="Error snap not found. ('$installed_criteria')"
                error "$resultDetails"
                log_error "$resultDetails"
                resultCode=0
                # ADUC_ERC_SCRIPT_HANDLER_INSTALL_INSTALLITEM_BAD_DATA, ERC Value: 810549762 (0x30500202)
                extendedResultCode=$((0x30500202))
                ret_val=0
            else
                log_error "Error installing snap: $response_body"
                resultDetails="Request failed (params: $snapd_api_data, status: $status_code)"
                error "$resultDetails"
                resultCode=0
                # ADUC_ERC_SCRIPT_HANDLER_INSTALL_INSTALLITEM_BAD_DATA, ERC Value: 810549762 (0x30500202)
                extendedResultCode=$((0x30500202))
                ret_val=0
            fi
            prepare_and_save_aduc_result $resultCode $extendedResultCode "$resultDetails"
            $ret $ret_val
        elif [ "$status_code" -ge 400 ]; then
            status_text=$(echo $response_body | jq -r ' . | .["status"]' )
            resultDetails="Install request error ('$installed_criteria', params: $snapd_api_data, status: $status_code)"
            resultCode=0
            # ADUC_ERC_SCRIPT_HANDLER_INSTALL_INSTALLITEM_BAD_DATA, ERC Value: 810549762 (0x30500202)
            extendedResultCode=$((0x30500202))
            error "$resultDetails"
            log_error "$resultDetails"
            if [ $status_code == 400 ]; then info "Tip: verify that revision and channel are correct."; fi
            ret_val=1
        else
            log_info "Snap ('$installed_criteria') isntallation request accepted."

            log_info "Wait for the snap package to be install..."

            waited_seconds=0
            install_inprogress="yes"
            while [ ! -z "$install_inprogress" ]; do
                sleep 1
                waited_seconds=$(( $waited_seconds + 1))

                # The installation is asynchronous.
                # So, post the install request again and check the result body for the install status.
                # NOTE: For somereason, calling response_body=$( $request_command ) here result in 404.
                response_body_2=$( curl -sX POST --unix-socket /run/snapd.socket "http://localhost/v2/snaps/$snap_name" -H "Content-Type: application/json" -d "$snapd_api_data" )

                # Sanitize response by removing text (if any) befor the first curly brace.
                brace_pos=$(expr index "$response_body_2" "{")
                if [ $brace_pos > 1 ]; then
                    response_body_2=${response_body_2:$brace_pos-1}
                    info "Installation status ('conflict' mean in-progress)\n$response_body_2"
                fi

                # If response body contains "snap-change-conflict", it means the installation is inprogress.
                if echo "$response_body_2" | grep -q '"kind":"snap-change-conflict"'; then
                    info "status: in progress..."
                    log_info "Installation in progress..."
                # If response body contains "snap-already-installed", it means the installation is completed.
                elif echo "$response_body_2" | grep -q '"kind":"snap-already-installed"'; then
                    info "status: completed"
                    log_info "Installation completed!"
                    resultCode=600
                    extendedResultCode=0
                    resultDetails=""
                    install_inprogress=
                elif echo "$response_body_2" | grep -q  '"status":"Not Found"'; then
                    resultDetails="Unexpected behavior. Abort the installation."
                    error "$resultDetails"
                    log_error "$resultDetails"
                    resultCode=0
                    #define ADUC_ERC_SCRIPT_HANDLER_APPLY_FAILURE_UNKNOWNEXCEPTION 810550271 (0x305003ff)
                    extendedResultCode=$((0x305003ff))
                    install_inprogress=
                elif [ $waited_seconds -gt $max_install_wait_time ]; then
                    resultDetails="Max installation wait time reached."
                    error "$resultDetails"
                    log_error "$resultDetails"
                    resultCode=0
                    #define ADUC_ERC_SCRIPT_HANDLER_APPLY_FAILURE_UNKNOWNEXCEPTION 810550271 (0x305003ff)
                    extendedResultCode=$((0x305003ff))
                    install_inprogress=
                fi
            done
        fi
        ret_val=0
    else
        resultDetails="The specified snap ('$installed_criteria') is already installed."
        info "$resultDetails"
        log_info "$resultDetails"
        resultCode=603
        extendedResultCode=0
        ret_val=0
    fi

    prepare_and_save_aduc_result "$resultCode" "$extendedResultCode" "$resultDetails"
    $ret $ret_val
}

#
# Executing the update finalization steps, but usually not required as the snap has already been
# installed by the 'install' action. When this function is completed,
# it will return a result code of 700, indicating that the 'apply' action was successful.
#
ApplyUpdate() {
    # 700 - ADUC_Result_Apply_Success
    log_info "Apply action is not required. Returning 700."

    ret_val=0
    resultCode=700
    extendedResultCode=0
    resultDetails="Apply action is not required."
    prepare_and_save_aduc_result "$resultCode" "$extendedResultCode" "$resultDetails"
    $ret $ret_val
}

#
# Cancel current update.
#
# Cancellation of the update is not possible. If the snap has already been updated or installed
# due to the update, it cannot be undone.
#
CancelUpdate() {
    # 801 - ADUC_Result_Cancel_UnableToCancel
    log_info "Cancelling the update is not possible. Returning 801."

    resultCode=801
    extendedResultCode=0
    resultDetails="Cancel action is not supported."
    ret_val=0
    prepare_and_save_aduc_result "$resultCode" "$extendedResultCode" "$resultDetails"
    $ret $ret_val
}

#
# Main
#

# Ensure that all required value is set.
if [ -z "$installed_criteria" ]; then
    resultDetails="Installed criteria is empty or missing."
    log_error "$resultDetails"
    # ADUC_ERC_SCRIPT_HANDLER_MISSING_INSTALLED_CRITERIA (0x30500002)
    extendedResultCode=$((0x30500002))
    prepare_and_save_aduc_result "0" "$extendedResultCode" "$resultDetails"
    exit $ret_val
fi

if [ -n "$check_is_installed" ]; then
    CheckIsInstalledState "$installed_criteria"
    exit $ret_val
fi

if [ -n "$do_download_action" ]; then
    DownloadUpdateArtifacts
    exit $ret_val
fi

if [ -n "$do_install_action" ]; then
    InstallUpdate "$installed_criteria"
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
