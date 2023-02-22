#!/bin/bash
# -------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See License.txt in the project root for
# license information.
# --------------------------------------------------------------------------
# Setup Script for Test: Ubuntu-18.04-amd64-APT-deployment
#
# Should be run on the Virtual Machine being provisioned for the work.

# This file should be included in an archive that is created at provisioning time
# with the following structure:
#       setup.sh (this file)
#       deviceupdate-package.deb (debian package under test)
#       <other-supporting-files>
# where the archive is named:
#       testsetup.tar.gz
# The archive is manually mounted to the file system at:
#       ~/testsetup.tar.gz
# Once the script has been extracted we'll be running from
# ~ while the script itself is at
#       ~/testsetup/setup.sh
# So we need to localize the path to that.
#

#
# Handling Self-Upgrade Scenario differently
#

self_upgrade=false
print_help() {
    echo "setup.sh [-s]  "
    echo "-s, --self-upgrade    Testing the self upgrade scenario, setup is different."
    echo "-h, --help            Show this help message."
}

while [[ $1 != "" ]]; do
    case $1 in
    -s | --self-upgrade)
        self_upgrade=true
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

#
# Install the Microsoft APT repository
#
function configure_apt_repository() {
    wget https://packages.microsoft.com/config/ubuntu/18.04/multiarch/packages-microsoft-prod.deb -O packages-microsoft-prod.deb
    sudo dpkg -i packages-microsoft-prod.deb
    rm packages-microsoft-prod.deb
    #
    # Update the apt repositories
    #
    sudo apt-get update
}

#
# Install Device Update Dependencies from APT
#
# Note: If there are other dependencies tht need to be installed via APT or other means they should
# be added here. You might be installing iotedge, another package to setup for a deployment test, or
# anything else.

function install_do() {
    # Handle installing DO from latest build instead of packages.microsoft.com
    wget https://github.com/microsoft/do-client/releases/download/v1.0.0/ubuntu1804_x64-packages.tar -O ubuntu18_x64-packages.tar
    tar -xvf ubuntu18_x64-packages.tar
    sudo apt-get install -y ./deliveryoptimization-agent_1.0.0_amd64.deb ./deliveryoptimization-plugin-apt_0.5.1_amd64.deb ./libdeliveryoptimization_1.0.0_amd64.deb
}

function register_extensions() {
    mkdir ~/adu_srcs/

    tar -xvf ./testsetup/adu_srcs_repo.tar.gz -C ~/adu_srcs/

    sudo mkdir -p ~/demo/demo-devices/contoso-devices

    cd ~/adu_srcs/src/extensions/component_enumerators/examples/contoso_component_enumerator/demo || exit

    chmod 755 ./tools/reset-demo-components.sh

    #copy components-inventory.json and adds it to the /usr/local/contoso-devices folder
    sudo cp -a ./demo-devices/contoso-devices/. ~/demo/demo-devices/contoso-devices/

    sh ./tools/reset-demo-components.sh

    #registers the extension
    sudo /usr/bin/AducIotAgent -l 2 --extension-type componentEnumerator --register-extension /var/lib/adu/extensions/sources/libcontoso_component_enumerator.so

    sh ./tools/reset-demo-components.sh
}

function verify_user_group_permissions() {
    # Get the UID and GID for the "adu" user
    adu_uid=$(id -u adu)
    adu_gid=$(id -g adu)

    # Find the main PID of the "AducIotAgent" service
    sudo systemctl start deviceupdate-agent.service
    status_output=$(systemctl status deviceupdate-agent.service)

    main_pid=$(echo "$status_output" | awk '/Main PID/ {print $3}')

    # Check if the user and group for the process are "adu"
    process_user=$(stat -c %u "/proc/$main_pid")
    process_group=$(stat -c %g "/proc/$main_pid")

    if [ "$process_user" -eq "$adu_uid" ] && [ "$process_group" -eq "$adu_gid" ]; then
        echo "User and group for AducIotAgent service are adu:adu."
    else
        echo "User and group for AducIotAgent service are not adu:adu."
        exit 1
    fi
}

function verify_log_files() {
    log_dir="/var/log/adu"
    if [ -d "$log_dir" ]; then
        log_files=$(ls "$log_dir")
        for log_file in $log_files; do
            log_path="$log_dir/$log_file"
            if [ -f "$log_path" ]; then
                continue
            else
                echo "$log_path is not a regular file."
                exit 1
            fi
        done
    else
        echo "$log_dir does not exist."
        exit 1
    fi
}

function test_shutdown_service() {
    # Stop the service
    sudo systemctl stop deviceupdate-agent.service

    # Check the service status
    timeout=5 # Timeout in seconds
    start_time=$(date +%s)
    while true; do
        # Check the service status
        status=$(systemctl status deviceupdate-agent.service)
        # Check if the service is in a failed state
        if echo "$status" | grep -q "failed"; then
            echo "Service failed to shut down."
            exit 1
        fi

        daemon_status=$(pgrep AducIotAgent)

        # Check if the daemon has been killed
        if [ -z "$daemon_status" ]; then
            echo "The Daemon has been killed successfully."
            return
        fi

        # Check if the timeout has been exceeded
        if [ $(($(date +%s) - start_time)) -gt $timeout ]; then
            echo "Timeout exceeded."
            exit 1
        fi

        # Wait for a bit before checking again
        sleep 1
    done
}

configure_apt_repository
#
# Install the Device Update Artifact Under Test
#

if [[ $self_upgrade == "true" ]]; then
    sudo apt-get install deviceupdate-agent -Y
    chmod u+x  ./apt_repo_setup.sh
    ./apt_repo_setup.sh -d ./testsetup/deviceupdate-package.deb
else
    sudo apt-get install -y ./testsetup/deviceupdate-package.deb
fi

#
# Install the du-config.json so the device can be provisioned
#
# Note: In other setup scripts there may be more info that needs to
# go here. For instance you might have the config.toml for IotEdge,
# another kind of diagnostics file, or other kinds of data
# this is the area where such things can be added
sudo cp ./testsetup/du-config.json /etc/adu/du-config.json

install_do

register_extensions

#
# Restart the deviceupdate-agent.service
#
# Note: We expect that everything should be setup for the deviceupdate agent at this point. Once
# we restart the agent we expect it to be able to boot back up and connect to the IotHub. Otherwise
# this test will be considered a failure.
sudo systemctl restart deviceupdate-agent.service

verify_user_group_permissions

verify_log_files

test_shutdown_service

sudo systemctl restart deviceupdate-agent.service
