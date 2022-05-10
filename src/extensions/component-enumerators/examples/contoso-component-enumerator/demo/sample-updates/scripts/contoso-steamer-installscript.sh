#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Ensure that getopt starts from first option if ". <script.sh>" was used.
OPTIND=1

ret_val=0

# Ensure we dont end the user's terminal session if invoked from source (".").
if [[ $0 != "${BASH_SOURCE[0]}" ]]; then
    ret=return
else
    ret=exit
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

# Log warning prefix - yello
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

#
# Install poicy
#
#install_error_policy="abort"
#install_reboot_policy="none"

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

restart_to_apply=
restart_agent_to_apply=

#
# Test or Demo related variables
#
simulate_preinstall_success=
simulate_postinstall_success=
reset_component=

#
# Remaining aguments and parameters
#
PARAMS=

#
# Output, Logs, and Result helper functions.
#
_timestamp=

update_timestamp()
{
    # See https://man7.org/linux/man-pages/man1/date.1.html
    _timestamp="$(date +'%Y/%m/%d:%H%M%S')"
}

log_debug(){
    if [ $log_level -gt 0  ]; then
        return
    fi
    log "$log_debug_pref" "$@"
}

log_info(){
    if [ $log_level -gt 1  ]; then
        return
    fi
    log "$log_info_pref" "$@"
}

log_warn(){
    if [ $log_level -gt 2  ]; then
        return
    fi
    log "$log_warn_pref" "$@"
}

log_error(){
    if [ $log_level -gt 3  ]; then
        return
    fi
    log "$log_error_pref" "$@"
}

log(){
    update_timestamp
    if [ -z $log_file ]; then
        echo -e "[$_timestamp]" "$@" >&1
    else
        echo "[$_timestamp]" "$@" >> $log_file
    fi
}

output(){
    update_timestamp
    if [ -z $output_file ]; then
        echo "[$_timestamp]" "$@" >&1
    else
        echo "[$_timestamp]" "$@" >> "$output_file"
    fi
}

result(){
    # NOTE: dont' insert timestamp in result file.
    if [ -z $result_file ]; then
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
    echo "--action-isinstalled                      Perform 'is-installed' check."
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
    echo "--install-error-policy                    Indicates how to proceed when an error occurs. Default \"abort\""
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
    echo "File and Folderinformation"
    echo "=========================="
    echo ""
    echo "--work-folder            A work-folder (or sandbox folder)."
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
        shift
        ;;

    --component-manufacturer | --component-vendor)
        if [[ -z $2 || $2 == -* ]]; then
            error "$1 parameter is mandatory."
            $ret 1
        fi
        component_vendor=$2
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
        shift
        ;;

    --component-version)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--component-version parameter is mandatory."
            $ret 1
        fi
        component_version=$1
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
        firmware_file="$1";
        echo "firmware file: $firmware_file"
        shift
        ;;

    --work-folder)
        shift
        if [[ -z $1 || $1 == -* ]]; then
            error "--work-folder parameter is mandatory."
            $ret 1
        fi
        workfolder="$1";
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
        output_file="$1";

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
        result_file="$1";
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
        log_file="$1";
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
    --reset-component)
        log_info "Will reset the specified component."
        reset_component=yes
        shift
        ;;

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
# Example implementation of 'IsInstalled' function, for contoso-motor component.
#
# Design Goal:
#   Determine whether the spcifified 'installedCriteria' (parameter $1) is met.
#
#   'installedCriteria' is a version number of a motor in a mock component's data file
#   located at "$component_props['path']/firmware.json".
#
#   The version number is in the following format:
#
#       {
#          ...
#          "version":"1.0"
#          ...
#       }
#
#
#   For demo purposes, to determine whether the motor has a desired version installed, we'll simply check
#   an output from command grep -F "\"version\":\"$installed_criteria\"" <component file>
#   If matched, grep command will return '0' exit code. Otherwise, '1'.
#
# Expected resultCode:
#     ADUC_Result_Failure = 0,
#     ADUC_Result_IsInstalled_Installed = 900,     /**< Succeeded and content is installed. */
#     ADUC_Result_IsInstalled_NotInstalled = 901,  /**< Succeeded and content is not installed */
#
IsInstalled(){
    path_key=path
    component_path=${component_props[$path_key]}
    component_file_path="$component_path/firmware.json"
    log_info "IsInstalled(\"$1\"), path:\"$component_file_path\""

    if [ -z "$1" ]; then

        # function call failed, due to invalid input.
        ret_val=1
        # resultCode(0) == Failure.
        resultCode=0
        # extendedResultCode(12345) mock value.
        extendedResultCode=12345
        resultDetails="Invalid installedCriteria value."
    elif [[ -f "$component_file_path" ]]; then

        log_debug "Found component data file '$component_file_path'."

        # check if version number matched.
        grep_params="-F \"\\\"version\\\": \\\"$installed_criteria$output_file\\\"\" '$component_file_path'"
        log_info "Running: grep $grep_params"

        {
            grep -F "\"version\": \"${installed_criteria}\"" "${component_file_path}"
        }

        grep_ret=$?
        log_info "grep exit code: $grep_ret"

        # Tell caller that IsInstall() function call succeeded.
        ret_val=0

        # resultCode, based on exit code from 'grep'.
        if [ $grep_ret -eq 0 ]; then
            # ADUC_Result_IsInstalled_Installed = 900
            resultCode=900
            # No additional error.
            extendedResultCode=0
            resultDetails=
        else
            # ADUC_Result_IsInstalled_NotInstalled = 901
            resultCode=901
            # No additional error.
            extendedResultCode=0
            resultDetails="installedCriteria not met: '$installed_criteria'."
        fi

    else
        #
        # Cannot get component data?
        # Fo this demo purposes, let's simulate the situation when this component is off, disconnected (not-pnp), or malfunction.
        #
        # Applicable extendedResultCode: (facility: 0x30000000)
        #   ADUC_ERC_UPDATE_CONTENT_HANDLER_ISINSTALLED_FAILURE_COMPONENT_UNAVAILABLE (502) - 0x1F6
        #   ADUC_ERC_UPDATE_CONTENT_HANDLER_ISINSTALLED_FAILURE_ACCESS_DATA (503)- 0x1F7
        #

        # function call succeeded.
        ret_val=0

        # resultCode(0) == Failure. Can't access device data.
        resultCode=0
        # extendedResultCode(0x300001F6)
        extendedResultCode=$((0x300001F6))
        resultDetails="Cannot communicate with the specified component. Name:$component_name, Id:$component_id"
    fi

    mock_result="{\"resultCode\":$resultCode, \"extendedResultCode\":$extendedResultCode,\"resultDetails\":\"$resultDetails\"}"

    # Show output.
    output "Result:" "$mock_result"

    # Write ADUC_Result to result file.
    result  "$mock_result"

}

#
# Helper function
# Not returning ADUC_RESULT
#
ResetComponent(){
    path_key=path
    component_path=${component_props[$path_key]}
    component_file_path="$component_path/firmware.json"
    log_info "Reset component($component_name), path:\"$component_file_path\""

    if [ -z "$component_path" ]; then
        log_error "Must specified --component-prop path <component_path>."
        ret_val=1
        $ret $ret_val
    fi

    if [ -d "$component_path" ]; then
        log_info "Path '$component_path' exists."
    else
        {
            mkdir -p "${component_path}"
            mkdir_ret=$?
            if [ $mkdir_ret -ne 0 ]; then
                log_error "Cannot create component data folder: '$component_path'."
                ret_val=$mkdir_ret
                $ret $ret_val
            fi
        }
    fi

    o="$component_file_path"

    {
        echo "{";
        echo "    \"id\": \"$component_id\"," ;
        echo "    \"name\": \"$component_name\"," ;
        echo "    \"group\": \"$component_group\"," ;
        echo "    \"manufacturer\": \"$component_vendor\"," ;
        echo "    \"model\": \"$component_model\"," ;
        echo "    \"version\": \"$component_version\"," ;
        echo "    \"description\": \"This component is generated for testing purposes.\"," ;
        echo "    \"properties\": {" ;
        echo "        \"path\": \"$component_path\"" ;
        echo "    }" ;
        echo "}"
    } > "$o"


    echo "#"
    echo "# Genrated component data file: \"$component_file_path\""
    echo "#"
    echo "----- content begin -----"
    {
        cat "${component_file_path}"
    }
    echo "----- content end -----"
    echo ""

    ret_val=0
}

DownloadUpdateArtifacts() {
    log_info "DownloadUpdateArtifacts called"
    path_key=path
    component_path=${component_props[$path_key]}
    component_file_path="$component_path/firmware.json"
    log_info "Note: nothing to do here. Expectting that all artifacts already downloaded."
    log_info "DownloadUpdateArtifacts succeeded."

    # ADUC_Result_Download_Skipped_FileExists = 502
    resultCode=502
    # No additional errors.
    extendedResultCode=0
    resultDetails="installedCriteria not met: '$installed_criteria'."

    # Prepare ADUC_Result json.
    mock_result="{\"resultCode\":$resultCode, \"extendedResultCode\":$extendedResultCode,\"resultDetails\":\"$resultDetails\"}"

    # Show output.
    output "Result:" "$mock_result"

    # Write ADUC_Result to result file.
    result  "$mock_result"

    $ret $ret_val
}

#
# InstallUpdate:
# Copies a 'firmware.json' to component's folder (properties.path).
#
InstallUpdate() {
    path_key=path
    component_path=${component_props[$path_key]}
    component_file_path="$component_path/firmware.json"
    log_info "InstallUpdate called, component:$component_name, firmware: $component_file_path"

    #
    # Note: we could simulate 'component off-line' scenario here.
    #

    # Check whether the component is already installed the specified update...

    # check if version number matched.
    if [ "$installed_criteria" == "" ]; then
        log_error "Script call missing '--installed-criteria' argument."
        resultCode=0
        # ADUC_ERC_SCRIPT_HANDLER_MISSING_INSTALLED_CRITERIA (0x30600002)
        extendedResultCode=$((0x30600002))
        resultDetails="Script call missing '--installed-criteria' argument."
    else

        # Check whether the update has benn installed.
        grep_params="-F \"\\\"version\\\": \\\"$installed_criteria$output_file\\\"\" '$component_file_path'"
        log_info "Running: grep $grep_params"

        {
            grep -F "\"version\": \"${installed_criteria}\"" "${component_file_path}"
        }

        grep_ret=$?
        log_info "grep exit code: $grep_ret"

        # resultCode, based on exit code from 'grep'.
        if [ $grep_ret -eq 0 ]; then
            log_info "It appears that this component already installed the specified update."
            # ADUC_Result_Install_Skipped_UpdateAlreadyInstalled = 603
            resultCode=603
            # No additional error.
            extendedResultCode=0
            resultDetails=
        else

            log_info "Installing... (workfolder:$workfolder, firmware:$firmware_file)"
            cp -f "$workfolder/$firmware_file" "$component_path/firmware.json"
            copy_ret=$?

            if [ $copy_ret -ne 0 ]; then
                log_error "Cannot install a firmware to: '$component_path'. (exitCode:$copy_ret)"

                # Set result code and details
                resultCode=0
                # ADUC_ERC_SCRIPT_HANDLER_CHILD_PROCESS_FAILURE_EXITCODE(exitCode) (0x30601000 + exitCode)
                extendedResultCode=$((0x30601000 + copy_ret))
                resultDetails="Firmware installation failed. ComponentId: $component_id"
            else
                log_info "Install succeeded."

                # ADUC_Result_Install_Success = 600
                resultCode=600
                # No additional error.
                extendedResultCode=0
                resultDetails=
            fi
            ret_val=0
        fi
    fi

    # Prepare ADUC_Result json.
    mock_result="{\"resultCode\":$resultCode, \"extendedResultCode\":$extendedResultCode,\"resultDetails\":\"$resultDetails\"}"

    # Show output.
    output "Result:" "$mock_result"

    # Write ADUC_Result to result file.
    result  "$mock_result"

    $ret $ret_val
}

ApplyUpdate() {
    ret_val=0
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
        # No additional error.
    extendedResultCode=0
    resultDetails=
    # Prepare ADUC_Result json.
    mock_result="{\"resultCode\":$resultCode, \"extendedResultCode\":$extendedResultCode,\"resultDetails\":\"$resultDetails\"}"

    # Show output.
    output "Result:" "$mock_result"

    # Write ADUC_Result to result file.
    result  "$mock_result"

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
    result  "$mock_result"

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
    result  "$mock_result"

    $ret $ret_val
}

CancelUpdate(){
    ret_val=0
    $ret $ret_val
}

#
# Main
#

if [ "$reset_component" == "yes" ]; then
    ResetComponent
    $ret $ret_val
fi

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

