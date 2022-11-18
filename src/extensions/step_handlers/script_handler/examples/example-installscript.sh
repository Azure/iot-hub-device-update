#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Ensure that getopt starts from first option if ". <script.sh>" was used.
OPTIND=1

ret_val=0

# Ensure we don't end the user's terminal session if invoked from source (".").
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
# Component information
#
component_id=
component_name=
component_group=
component_vendor=
component_model=
component_version=


# component_props will contain a map of property.
declare -A component_props

#
# Files and Folders information
#
workfolder=
firmware_file=
output_file=
log_file=
result_file=

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
# Test or Demo related variables
#
simulate_preinstall_success=
simulate_postinstall_success=

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

#
# Write result json string to result file.
#
result() {
    # NOTE: don't insert timestamp in result file.
    if [ -z "$result_file" ]; then
        echo "$@" >&1
    else
        echo "$@" > "$result_file"
    fi
}

#
# Usage
#
print_help() {
    echo ""
    echo "Usage: <script-file-name>.sh [options...]"
    echo ""
    echo "Component information (required for component update)"
    echo "====================================================="
    echo ""
    echo "--component-id <component_id>             A component identifier."
    echo "--component-name <component_name>         A component name."
    echo "--component-vendor <component_vendor>     A component vendor or manufacturer."
    echo "--component-model <component_model>       A component model."
    echo "--component-version <component_version>   A component version."
    echo "--component-prop <prop_name> <prop_value> A name/value pair of component's additional property."
    echo "                                          This will be stored in the 'component_prop' map."
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
    echo '--install-error-policy                    Indicates how to proceed when an error occurs. Default "abort"'
    echo "                                          options: abort, continue"
    echo "--install-reboot-policy                   Indicates whether a device reboot is required, after install completed successfully."
    echo "                                          Optios:"
    echo "                                              none (default)"
    echo "                                              immediate - the agent should restart the device immediately."
    echo "                                              deferred  - the agent should restart the device whne workflow is completed."
    echo ""
    echo "Script Execution Mode"
    echo "====================="
    echo ""
    echo "--pre-install-sim-success     Simulate pre-install success scenario (no-op)"
    echo "--install                     Execute this script as an 'installer'"
    echo "--post-install-sim-success    Simulate post-install success scenario (no-op)"
    echo ""
    echo ""
    echo "File and Folder information"
    echo "=========================="
    echo ""
    echo "--work-folder           A work-folder (or sandbox folder)."
    echo "--firmware-file         A firmware to install."
    echo "--output-file           An output file."
    echo "--log-file              A log file."
    echo "--result-file           A file contain ADUC_Result data (in JSON format)."
    echo ""
    echo "--log-level <0-4>             A minimum log level. 0=debug, 1=info, 2=warning, 3=error, 4=none."
    echo "-h, --help                    Show this help message."
    echo ""
    echo "For testing purposes only:"
    echo ""
    echo "--reset-component             Create component data file using specified component information."
    echo "                              Note: must specify --component-prop path <property file path>"
    echo "                              e.g.,   --component-prop path /usr/local/adu-tests/contoso-devices/vacumm-1/motos/contoso-motor-serial-0001"
    echo ""
    echo "Example:"
    echo ""
    echo "Reset (test) component"
    echo "======================"
    echo "    <script> --log-level 0 --component-group firmware --component-name host-fw --component-id 0 --component-prop path /usr/local/adu-tests/contoso-devices/vacuum-1/hostfw --component-vendor contoso --component-model virtual-vacuum --component-version 1.0 --reset-component"
    echo ""
    echo "Scenario: is-installed check (installed)"
    echo "========================================"
    echo "    <script> --log-level 0 --action-is-installed --intalled-criteria 1.0 --component-name host-fw --component-prop path /usr/local/adu-tests/contoso-devices/vacuum-1/hostfw"
    echo ""
    echo "Scenario: is-installed check (not installed)"
    echo "============================================"
    echo "    <script> --log-level 0 --action-is-installed --intalled-criteria FOO --component-name host-fw --component-prop path /usr/local/adu-tests/contoso-devices/vacuum-1/hostfw"
    echo ""
    echo "Scenario: component off-line"
    echo "============================"
    echo "    <script> --log-level 0 --action-is-installed --intalled-criteria 1.0 --component-name host-fw  --component-prop path /usr/local/adu-tests/contoso-devices/vacuum-1/hostfw"
    echo ""
    echo "Scenario: perform install action"
    echo "================================"
    echo "    <script> --log-level 0 --action-install --intalled-criteria 1.0 --component-name host-fw  --component-prop path /usr/local/adu-tests/contoso-devices/vacuum-1/hostfw --firmware-file firmware.json --work-folder <sandbox-folder>"
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
    # Component-related arguments.
    #
    # Following arguments and parameters will be populated by the Script Update Content Handler.
    #
    --component-id)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--component-id parameter is mandatory."
            $ret 1
        fi
        component_id=$1
        echo "Component id: $component_id"
        shift
        ;;

    --component-name)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--component-name parameter is mandatory."
            $ret 1
        fi
        component_name=$1
        echo "Component name: $component_name"
        shift
        ;;

    --component-group)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--component-group parameter is mandatory."
            $ret 1
        fi
        component_group=$1
        echo "Component group: $component_group"
        shift
        ;;

    --component-manufacturer | --component-vendor)
        if [[ -z $2 || $2 == -* ]]; then
            error "$1 parameter is mandatory."
            $ret 1
        fi
        component_vendor=$2
        echo "Component vendor: $component_vendor"
        shift
        shift
        ;;

    --component-model)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--component-model parameter is mandatory."
            $ret 1
        fi
        component_model=$1
        echo "Component model: $component_model"
        shift
        ;;

    --component-version)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--component-version parameter is mandatory."
            $ret 1
        fi
        component_version=$1
        echo "Component version: $component_version"
        shift
        ;;

    # (optional) - support component description, if needed.
    --component-prop)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--component-prop requires 2 parameters."
            $ret 1
        fi
        if [[ -z $2 || $2 == -* ]]; then
            error "--component-prop requires 2 parameters."
            $ret 1
        fi

        component_props["$1"]="$2"
        echo "Set component property '$1', value: '${component_props[$1]}'"

        shift
        shift
        ;;

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
    # Install policy
    #
    --install-error-policy)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--install-error-policy parameter is mandatory."
            $ret 1
        fi
        #install_error_policy="$1";
        shift
        ;;

    --install-reboot-policy)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--install-reboot-policy parameter is mandatory."
            $ret 1
        fi
        #install_error_policy="$1";
        shift
        ;;

    #
    # Update artifacts
    #
    --firmware-file)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--firmware-file parameter is mandatory."
            $ret 1
        fi
        firmware_file="$1"
        echo "firmware file: $firmware_file"
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

    --log-file)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--log-file parameter is mandatory."
            $ret 1
        fi
        log_file="$1"
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

    #
    # Test or Demo fuctionality
    #
    --pre-install-sim-success)
        log_info "Will simulate pre-install script success."
        simulate_preinstall_success=yes
        shift
        ;;

    --post-install-sim-success)
        log_info "Will simulate post-install script success."
        simulate_postinstall_success=yes
        shift
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
    resultCode=901
    extendedResultCode=0
    resultDetails=""
    ret_val=0

    #
    # PLACEHOLDER : Evalue 'installedCriteria'
    #
    #               Based on overall result..
    #
    #               # Set result code and details
    #               resultCode=<Result Code>
    #               extendedResultCode=<Extended Result Code, in case of error>
    #               resultDetails="<Additional result details>
    #               $ret_value=<Script exit code>

    # Prepare ADUC_Result json.
    aduc_result_json="{\"resultCode\":$resultCode, \"extendedResultCode\":$extendedResultCode,\"resultDetails\":\"$resultDetails\"}"

    # Show output.
    output "Result:" "$aduc_result_json"

    # Write ADUC_Result to result file.
    result "$aduc_result_json"

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
DownloadUpdateArtifacts() {
    resultCode=500
    extendedResultCode=0
    resultDetails=""
    ret_val=0

    #
    # PLACEHOLDER : Perform download-related tasks here.
    #
    #               Based on overall result..
    #
    #               # Set result code and details
    #               resultCode=<Result Code>
    #               extendedResultCode=<Extended Result Code, in case of error>
    #               resultDetails="<Additional result details>
    #               $ret_value=<Script exit code>

    # Prepare ADUC_Result json.
    aduc_result_json="{\"resultCode\":$resultCode, \"extendedResultCode\":$extendedResultCode,\"resultDetails\":\"$resultDetails\"}"

    # Show output.
    output "Result:" "$aduc_result_json"

    # Write ADUC_Result to result file.
    result "$aduc_result_json"

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
InstallUpdate() {
    resultCode=600
    extendedResultCode=0
    resultDetails=""
    ret_val=0

    #
    # PLACEHOLDER : Evaluate 'installCriteria' to determine if there is anything to do.
    #               If not, return ADUC_Result_Install_Success (600). Otherwise, continue.
    #

    #
    # PLACEHOLDER : Perform installation tasks here.
    #
    #               Based on overall result..
    #
    #               # Set result code and details
    #               resultCode=<Result Code>
    #               extendedResultCode=<Extended Result Code, in case of error>
    #               resultDetails="<Additional result details>
    #               $ret_value=<Script exit code>

    # Prepare ADUC_Result json.
    aduc_result_json="{\"resultCode\":$resultCode, \"extendedResultCode\":$extendedResultCode,\"resultDetails\":\"$resultDetails\"}"

    # Show output.
    output "Result:" "$aduc_result_json"

    # Write ADUC_Result to result file.
    result "$aduc_result_json"

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
ApplyUpdate() {
    resultCode=700
    extendedResultCode=0
    resultDetails=""
    ret_val=0
    #
    # PLACEHOLDER : Perform installation tasks here.
    #
    #               Based on overall result..
    #
    #               # Set result code and details
    #               resultCode=<Result Code>
    #               extendedResultCode=<Extended Result Code, in case of error>
    #               resultDetails="<Additional result details>
    #               $ret_value=<Script exit code>

    # Prepare ADUC_Result json.
    aduc_result_json="{\"resultCode\":$resultCode, \"extendedResultCode\":$extendedResultCode,\"resultDetails\":\"$resultDetails\"}"

    # Show output.
    output "Result:" "$aduc_result_json"

    # Write ADUC_Result to result file.
    result "$aduc_result_json"

    $ret $ret_val
}

SimulatePreInstallSuccess() {
    output "Simulating pre-instll step success."
    ret_val=0
    # ADUC_Result_Apply_Success = 700
    resultCode=700
    # No additional error.
    extendedResultCode=0
    resultDetails=
    # Prepare ADUC_Result json.
    mock_result="{\"resultCode\":$resultCode, \"extendedResultCode\":$extendedResultCode,\"resultDetails\":\"$resultDetails\"}"

    # Show output.
    output "Result:" "$mock_result"

    # Write ADUC_Result to result file.
    result "$mock_result"

    $ret $ret_val
}

SimulatePostInstallSuccess() {
    output "Simulating post-instll step success."
    ret_val=0
    # ADUC_Result_Apply_Success = 700
    resultCode=700
    # No additional error.
    extendedResultCode=0
    resultDetails=
    # Prepare ADUC_Result json.
    mock_result="{\"resultCode\":$resultCode, \"extendedResultCode\":$extendedResultCode,\"resultDetails\":\"$resultDetails\"}"

    # Show output.
    output "Result:" "$mock_result"

    # Write ADUC_Result to result file.
    result "$mock_result"

    $ret $ret_val
}

#
# Cancel the script execution.
#
# Expected resultCode:
#    ADUC_Result_Failure = 0,
#    ADUC_Result_Cancel_Success = 800,            /**< Succeeded. */
#    ADUC_Result_Cancel_UnableToCancel = 801,     /**< Not a failure. Cancel is best effort. */
CancelUpdate() {
    # EXAMPLE : Most script cannot be canceled. So this is no-op.

    # Write ADUC_Result to result file.
    result "{\"resultCode\":801, \"extendedResultCode\":0,\"resultDetails\":\"\"}"
    $ret 0
}

#
# Main
#
if [ -n "$check_is_installed" ]; then
    IsInstalled "$installed_criteria"
    exit $ret_val
fi

if [ -n "$simulate_preinstall_success" ]; then
    SimulatePreInstallSuccess
    exit $ret_val
fi

if [ -n "$simulate_postinstall_success" ]; then
    SimulatePostInstallSuccess
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
