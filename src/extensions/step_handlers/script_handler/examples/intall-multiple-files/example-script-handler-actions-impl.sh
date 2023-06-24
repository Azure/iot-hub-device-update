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
bold " THIS EXAMPLE UPDATE SCRIPT IS PROVIDED "AS IS", WITHOUT WARRANTY"
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

header "Example Script v-2023-06-23"

script_name="copy-files-demo"
script_version="v-2023-06-23"

info "INPUT PARAMETERS: $@"

#
# Files and Folders information
#
work_folder=
output_file=/var/log/adu/${script_name}.output
log_file=/var/log/adu/${script_name}.log
result_file=/var/log/adu/${scrpit_name}_result.json

#
# Example of some variables that can be used in the script.
#
# These variables are hard-coded for the example, but could be passed in as
# parameters to the script, by specifying them in the "handlerProperties" section of the import manifest.
#
# One example of the custom handlerProperties is 'outputDataBrickFile', which is the name of the output data brick file.
#

# A demo custome install folder to move files to
install_folder=/tmp/adu/script-handler-demo
data_file_1_name="data-file-1"
data_file_2_name="data-file-2"
data_brick_file_name="data-3.brick"
mkdir -p $install_folder

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

# Example usage:
#   prepare_and_save_aduc_result 0 0 "Operation succeeded"
#   This will wrap up the operation with result code 0, extended result code 0, and result details "Operation succeeded".
prepare_and_save_aduc_result() {
    result_code=$1
    extended_result_code=$2
    result_details=$3
    aduc_result=""

    # Prepare ADUC_Result json.
    make_aduc_result_json "$resultCode" "$extendedResultCode" "$resultDetails" aduc_result

    # Show output.
    output "Result:" "$aduc_result"

    # Write ADUC_Result to result file.
    result "$aduc_result"
}

this_file_name=$(basename "$0")

#
# Print usage message.
#
print_help() {
    echo ""
    echo "Usage: ${this_file_name} [options...]"
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
    echo "--action-download                 Perform 'download' aciton."
    echo "--action-install                  Perform 'install' action."
    echo "--action-apply                    Perform 'apply' action."
    echo "--action-cancel                   Perform 'cancel' action."
    echo ""
    echo "Example of a custome argument specified in 'handlerProperties' section of the import manifest:"
    echo "----------------------------------------"
    echo ""
    echo "--data-brick-file-name            Name of the output data brick file to be generated."
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

    --data-brick-file-name)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            log_error "--data-brick-file-name requires a file name parameter."
            $ret 1
        fi
        data_brick_file_name="$1"
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
# A helper function that evaluates whether the data-brick file has been generated.
# The expected data brick file name is specified by 'installedCriteria'.
#
# Usage: is_installed $data_brick_file_path <out result_code> <out extended_result_code> <out result_details>
#
# shellcheck disable=SC2034
function is_installed {
    local data_brick_file_path=$1
    local -n result_code=$2  # name reference for result_code
    local -n extended_result_code=$3 # name reference for extended_result_code
    local -n result_details=$4  # name reference for result_details

    # if the data_brick_file_path exists, and its content equal the installed criteria, then the package is installed.
    if [[ -f "$data_brick_file_path" ]]; then
        if [[ "$(cat "$data_brick_file_path")" == "$installed_criteria" ]]; then
            result_code=900
            extended_result_code=0
            result_details="Package is installed"
            return
        else
            result_code=901
            result_details="Package is not installed"
        fi
    else
        result_code=901
        result_details="$data_brick_file_path does not exist"
    fi
}

# This function determines whether the data-brick file, as specified 'installedCriteria' (parameter $1) does exist on the device.
#
#   'installedCriteria' is an output data brick file that is generated by the update process.
#
#    Example installed criteria string:
#         'data-3.brick'
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

    # Evaluates whether the data-brick file has been generated and contains the correct data.
    is_installed "$install_folder/$data_brick_file_name" local_rc local_erc local_rd

    prepare_and_save_aduc_result "$local_rc" "$local_erc" "$local_rd"
    ret_val=0
}

#
# No additional files to download, returns 500.
#
# Note: every files that are included as part of the current step will be downloaded by
#       the ScriptHandler before it invoking this script for --action-download (or --action download).
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
# Performs the installation of the update.
#
# This includes:
#  - copy data-1, data-2, process-data-file.sh from the work_foler to the install_folder (/usr/local/script-handler-demo/)
#  - run /usr/local/script-handler-demo/process-data-file.sh /usr/local/script-handler-demo/data-1 /usr/local/script-handler-demo/data-2 /usr/local/script-handler-demo/data-3.brick
#
InstallUpdate() {
    local installed_criteria=$1
    log_info "Begin an update installation task (installed_criteria:'$1')"

    resultCode=0
    extendedResultCode=0
    resultDetails=""
    ret_val=0

    # Check whether the output data brick file has been generated.
    # If so, then we can skip the installation.
    is_installed "$install_folder/$data_brick_file_name" resultCode extendedResultCode resultDetails

    is_installed_ret=$?

    if [[ $is_installed_ret -ne 0 ]]; then
        # is_installed function failed to execute.
        resultDetails="Internal error in 'is_installed' function."
        log_error "$resultDetails"
        resultCode=0
        make_script_handler_erc "$is_installed_ret" extendedResultCode
    elif [[ $resultCode == 0 ]]; then
        # Return current ADUC_Result
        resultDetails="Failed to determine wehther the data brick file has been generated."
        log_error "$resultDetails"
        resultCode=0
        make_script_handler_erc "$is_installed_ret" extendedResultCode
    elif [[ $resultCode -eq 901 ]]; then

        # The update has not been installed yet.
        # Let's copy the data files and run the process-data-file.sh script.
        cp $work_folder/$data_file_1_name $install_folder
        cp $work_folder/$data_file_2_name $install_folder

        # Run the process-data-file.sh script.
        echo "Running Command:" $work_folder/process-data-file.sh "$install_folder/$data_file_1_name" "$install_folder/$data_file_2_name" "$install_folder/$data_brick_file_name"
        $work_folder/process-data-file.sh "$install_folder/$data_file_1_name" "$install_folder/$data_file_2_name" "$install_folder/$data_brick_file_name"
        process_ret=$?

        # If the exit code is not 0, then we can set the resultCode to 0 and set the extendedResultCode to the exit code of the process-data-file.sh script.
        if [[ $process_ret -ne 0 ]]; then
             resultCode=0
             make_script_handler_erc "$process_ret" extendedResultCode
             resultDetails="Failed to run the process-data-file.sh script."
        else
            resultCode=600
            extendedResultCode=0
            resultDetails=""
            ret_val=0
        fi
    else
        resultDetails="The specified data brick ('$installed_criteria') is already installed."
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
# Executing the update finalization steps, but for this demo purpose, we don't have any.
#
# This will return a result code of 700, indicating that the 'apply' action was successful.
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
# Cancellation of the update is not supported for this demo.
#
CancelUpdate() {
    # 801 - ADUC_Result_Cancel_UnableToCancel
    log_info "For this demon, cancelling the update is note supported. Returning 801."

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

# If work folder is not speicifed, set work folder to the same as this script location (at runtime)
if [ -z "$work_folder" ]; then
    work_folder="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
    warn "Work folder is not specified. Using '$work_folder' as work folder."
else
    # echo work folder
    echo "Work folder is '$work_folder'"
fi

#if log file is not specified, set log file as work_folder/_default.log
if [ -z "$log_file" ]; then
    log_file="$work_folder/_default.log"
    warn "Log file is not specified. Using '$log_file' as log file."
else
    # echo log file
    echo "Log file is '$log_file'"
fi

#if output file is not specified, set output file as work_folder/_default.out
if [ -z "$output_file" ]; then
    output_file="$work_folder/_default.out"
    warn "Output file is not specified. Using '$output_file' as output file."
else
    # echo output file
    echo "Output file is '$output_file'"
fi

#if result file is not specified, set result file as work_folder/_default_aduc_result.json
if [ -z "$result_file" ]; then
    result_file="$work_folder/_default_aduc_result.json"
    warn "Result file is not specified. Using '$result_file' as result file."
else
    # echo result file
    echo "Result file is '$result_file'"
fi

# Ensure that all required value is set.
if [ -z "$installed_criteria" ]; then
    echo "ERROR: Installed criteria is empty or missing."
    resultDetails="Installed criteria is empty or missing."
    log_error "$resultDetails"
    # ADUC_ERC_SCRIPT_HANDLER_MISSING_INSTALLED_CRITERIA (0x30500002)
    extendedResultCode=$((0x30500002))
    prepare_and_save_aduc_result "0" "$extendedResultCode" "$resultDetails"
    exit $ret_val
else
    echo "Installed criteria is '$installed_criteria'"
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
