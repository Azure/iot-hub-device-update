#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Ensure that getopt starts from first option if ". <script.sh>" was used.
OPTIND=1

ret_val=0

# Ensure we don't end the user's terminal session when invoked from source (".").
if [[ $0 != "${BASH_SOURCE[0]}" ]]; then
    ret='return'
else
    ret='exit'
fi

# Files and Folders information
output_file=
result_file=
workfolder=

# Device Update specific arguments
check_is_installed=
installed_criteria=
do_download_action=
do_install_action=
do_apply_action=
do_cancel_action=

# Remaining aguments and parameters
PARAMS=

#
# Helper functions.
#

# Write result json string to result file.
SaveResult() {
    if [ -z "$result_file" ]; then
        echo "$@" >&1
    else
        echo "$@" > "$result_file"
    fi
}

#
# Device Update related functions.
#

#
# Determines whether the specified 'installedCriteria' (parameter $1) is met.
#
# Requried argument:
#   'installedCriteria' is a data that used for evaluating whether this script should be exectued.
#
# Expected resultCode:
#    ADUC_Result_Failure = 0,
#    ADUC_Result_IsInstalled_Installed = 900,     /**< Succeeded and content is installed. */
#    ADUC_Result_IsInstalled_NotInstalled = 901,  /**< Succeeded and content is not installed */
#
IsInstalled() {
    # Returns ADUC_Result_IsInstalled_NotInstalled to communicate to caller that this script should be executed.
    resultCode=901
    extendedResultCode=0
    resultDetails=""
    ret_val=0

    # Prepare ADUC_Result json.
    aduc_result_json="{\"resultCode\":$resultCode, \"extendedResultCode\":$extendedResultCode,\"resultDetails\":\"$resultDetails\"}"

    # Write ADUC_Result to result file.
    SaveResult "$aduc_result_json"

    $ret $ret_val
}

#
# Downloads additional files.
#
#
# Expected resultCode:
#    ADUC_Result_Failure = 0,
#    ADUC_Result_Download_Success = 500,                  /**< Succeeded. */
#    ADUC_Result_Download_InProgress = 501,               /**< Async operation started. CompletionCallback will be called when complete. */
#    ADUC_Result_Download_Skipped_FileExists = 502,       /**< Download skipped. File already exists and hash validation passed. */
#
#    ADUC_Result_Download_Skipped_UpdateAlreadyInstalled = 503, /**< Download succeeded. Also indicates that the Installed Criteria is met. */
#    ADUC_Result_Download_Skipped_NoMatchingComponents = 504, /**< Download succeeded. Also indicates that no matchings components for this update. */
#
#    ADUC_Result_Download_Handler_SuccessSkipDownload = 520,  /**< Succeeded. DownloadHandler was able to produce the update. Agent must skip downloading. */
#    ADUC_Result_Download_Handler_RequiredFullDownload = 521, /**< Not a failure. Agent fallback to downloading the update is required. */
#
DoDownload() {
    # EXAMPLE: Return ADUC_Result_Download_Success since no additional downloads is needed.
    resultCode=500
    extendedResultCode=0
    resultDetails=""
    ret_val=0

    #
    # PLACEHOLDER: In some scenario, an additional content is required and need to be downloaded on-demand.
    #              If that the case, it can be done here.
    #

    # Prepare ADUC_Result json.
    aduc_result_json="{\"resultCode\":$resultCode, \"extendedResultCode\":$extendedResultCode,\"resultDetails\":\"$resultDetails\"}"

    # Write ADUC_Result to result file.
    SaveResult "$aduc_result_json"

    $ret $ret_val
}

#
# Perform install-related tasks.
#
# Expected resultCode:
#    ADUC_Result_Failure = 0,
#    ADUC_Result_Install_Success = 600,                        /**< Succeeded. */
#    ADUC_Result_Install_Skipped_UpdateAlreadyInstalled = 603, /**< Succeeded (skipped). Indicates that 'install' action is no-op. */
#    ADUC_Result_Install_RequiredImmediateReboot = 605,        /**< Succeeded. An immediate device reboot is required, to complete the task. */
#    ADUC_Result_Install_RequiredReboot = 606,                 /**< Succeeded. A deferred device reboot is required, to complete the task. */
#    ADUC_Result_Install_RequiredImmediateAgentRestart = 607,  /**< Succeeded. An immediate agent restart is required, to complete the task. */
#    ADUC_Result_Install_RequiredAgentRestart = 608,           /**< Succeeded. A deferred agent restart is required, to complete the task. */
#
DoInstall() {
    # EXAMPLE: Return ADUC_Result_Install_Success
    resultCode=600
    extendedResultCode=0
    resultDetails=""
    ret_val=0

    #
    # PLACEHOLDER: Add desired 'install' scripts here.
    #
    #               # Set result code and details
    #               resultCode=<Result Code>
    #               extendedResultCode=<Extended Result Code, in case of error>
    #               resultDetails="<Additional result details>
    #               $ret_value=<Script exit code>

    # Prepare ADUC_Result json.
    aduc_result_json="{\"resultCode\":$resultCode, \"extendedResultCode\":$extendedResultCode,\"resultDetails\":\"$resultDetails\"}"

    # Write ADUC_Result to result file.
    SaveResult "$aduc_result_json"

    $ret $ret_val
}

#
# Perform optional tasks to finalize the update.
#
# Expected resultCode:
#    ADUC_Result_Failure = 0,
#    ADUC_Result_Apply_Success = 700,                         /**< Succeeded. */
#    ADUC_Result_Apply_RequiredImmediateReboot = 705,         /**< Succeeded. An immediate device reboot is required, to complete the task. */
#    ADUC_Result_Apply_RequiredReboot = 706,                  /**< Succeeded. A deferred device reboot is required, to complete the task. */
#    ADUC_Result_Apply_RequiredImmediateAgentRestart = 707,   /**< Succeeded. An immediate agent restart is required, to complete the task. */
#    ADUC_Result_Apply_RequiredAgentRestart = 708,            /**< Succeeded. A deferred agent restart is required, to complete the task. */
#
DoApply() {
    # EXAMPLE: Return ADUC_Result_Apply_Success
    resultCode=700
    extendedResultCode=0
    resultDetails=""
    ret_val=0

    #
    # PLACEHOLDER : Add any desired scripts to finalize the installtion here.
    #

    # Prepare ADUC_Result json.
    aduc_result_json="{\"resultCode\":$resultCode, \"extendedResultCode\":$extendedResultCode,\"resultDetails\":\"$resultDetails\"}"

    # Write ADUC_Result to result file.
    SaveResult "$aduc_result_json"

    $ret $ret_val
}

#
# Cancel the script execution.
#
# Expected resultCode:
#    ADUC_Result_Failure = 0,
#    ADUC_Result_Cancel_Success = 800,            /**< Succeeded. */
#    ADUC_Result_Cancel_UnableToCancel = 801,     /**< Not a failure. Cancel is best effort. */
DoCancel() {
    # EXAMPLE : Most script can't be cancel. So this is no-op.

    # Write ADUC_Result to result file.
    SaveResult "{\"resultCode\":801, \"extendedResultCode\":0,\"resultDetails\":\"\"}"

    $ret 0
}


#
# Main
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
        do_install_action=yes
        ;;

    --action-apply)
        shift
        do_apply_action=yes
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
            echo "ERROR: --installed-criteria requires an installedCriteria parameter."
            $ret 1
        fi
        installed_criteria="$1"
        shift
        ;;


    --work-folder)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--work-folder parameter is mandatory."
            $ret 1
        fi
        workfolder="$1"
        echo "Workfolder: $workfolder"
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
        # Delete existing output.
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
        # Create result file path.
        #
        # Delete existing result files.
        rm -f -r "$result_file"
        # Create dir(s) recursively (include filename, well remove it in the following line...).
        mkdir -p "$result_file"
        # Delete leaf-dir (w)
        rm -f -r "$result_file"
        shift
        ;;

    *) # preserve positional arguments
        PARAMS="$PARAMS $1"
        shift
        ;;
    esac
done


# Perform action
if [ -n "$check_is_installed" ]; then
    IsInstalled "$installed_criteria"
    exit $ret_val
fi

if [ -n "$do_download_action" ]; then
    DoDownload
    exit $ret_val
fi

if [ -n "$do_install_action" ]; then
    DoInstall
    exit $ret_val
fi

if [ -n "$do_apply_action" ]; then
    DoApply
    exit $ret_val
fi

if [ -n "$do_cancel_action" ]; then
    DoCancel
    exit $ret_val
fi

$ret $ret_val
