#!/bin/bash
# -------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See License.txt in the project root for
# license information.
# --------------------------------------------------------------------------
# Setup Script for Test
#
# Should be run on the Virtual Machine being provisioned for the work.

# This file should be included in an archive that is created at provisioning time
# with the following structure:
#       x509_vm_setup.sh (this file)
#       deviceupdate-package.deb (debian package under test)
#       <other-supporting-files>
# where the archive is named:
#       testsetup.tar.gz
# The archive is manually mounted to the file system at:
#       ~/testsetup.tar.gz
# Once the script has been extracted we'll be running from
# ~ while the script itself is at
#       ~/testsetup/x509_vm_setup.sh
# So we need to localize the path to that.
#

print_help() {
    echo "Usage: x509_vm_setup.sh [OPTIONS]"
    echo "Options:"
    echo "  -d, --distro       Distribution and version (e.g. ubuntu-18.04, debian-10)"
    echo "  -a, --architecture Architecture (e.g. amd64, arm64)"
    echo "  -s, --self-upgrade Testing the self upgrade scenario, setup is different."
    echo "  -h, --help         Show this help message."
}

distro=""
architecture=""
self_upgrade=false

while [[ $1 != "" ]]; do
    case $1 in
    -d | --distro)
        shift
        distro=$1
        ;;
    -a | --architecture)
        shift
        architecture=$1
        ;;
    -s | --self-upgrade)
        self_upgrade=true
        ;;
    -h | --help)
        print_help
        exit 0
        ;;
    *)
        echo "Invalid argument: $*"
        print_help
        exit 1
        ;;
    esac
    shift
done

if [ -z "$distro" ] || [ -z "$architecture" ]; then
    echo "Error: Both distro and architecture must be provided"
    print_help
    exit 1
fi

#
# Install Device Update Dependencies from APT
#
# Note: If there are other dependencies that need to be installed via APT or other means they should
# be added here. You might be installing iotedge, another package to setup for a deployment test, or
# anything else.

# Handle installing DO from latest build instead of packages.microsoft.com
function configure_apt_repository() {
    local distro_full=$1
    local architecture=$2
    local package_url=""

    local distro
    distro=$(echo "$distro_full" | cut -d'-' -f1)
    local version
    version=$(echo "$distro_full" | cut -d'-' -f2)

    if [ "$distro" == "ubuntu" ]; then
        if [ "$version" == "18.04" ]; then
            package_url="https://packages.microsoft.com/config/ubuntu/18.04/multiarch/packages-microsoft-prod.deb"
        elif [ "$version" == "20.04" ]; then
            package_url="https://packages.microsoft.com/config/ubuntu/20.04/packages-microsoft-prod.deb"
        elif [ "$version" == "22.04" ]; then
            package_url="https://packages.microsoft.com/config/ubuntu/22.04/packages-microsoft-prod.deb"
        fi
    elif [ "$distro" == "debian" ] && [ "$version" == "10" ]; then
        package_url="https://packages.microsoft.com/config/debian/10/packages-microsoft-prod.deb"
    fi

    if [ -n "$package_url" ]; then
        wget "$package_url" -O packages-microsoft-prod.deb
        sudo dpkg -i packages-microsoft-prod.deb
        rm packages-microsoft-prod.deb

        # Update the apt repositories
        sudo apt-get update
    else
        echo "Unsupported distro or version"
        exit 1
    fi
}

#
# Installs the certificates and private keys into /etc/prov_info
#
function install_certs_and_keys() {
    if [ -d "/etc/prov_info" ]; then
        sudo rm -rf /etc/prov_info
    fi

    sudo mkdir /etc/prov_info

    sudo cp ./testsetup/*-primary.pem /etc/prov_info/
    sudo cp ./testsetup/*-secondary.pem /etc/prov_info/

    sudo cp ./testsetup/*-primary-key.pem /etc/prov_info/
    sudo cp ./testsetup/*-secondary-key.pem /etc/prov_info/
}

#
# Installs and Configures AIS with the pre-generated config.toml
#
function install_and_configure_ais() {
    sudo apt-get install -y aziot-identity-service
    sudo cp ./testsetup/config.toml /etc/aziot/config.toml
    sudo aziotctl config apply -c /etc/aziot/config.toml
    sudo aziotctl check
}

#
# Install Device Update Dependencies from APT
#
# Note: If there are other dependencies that need to be installed via APT or other means they should
# be added here. You might be installing iotedge, another package to setup for a deployment test, or
# anything else.

function install_do() {
    local distro_full=$1
    local architecture=$2
    local package_url=""
    local package_filename=""

    local distro
    distro=$(echo "$distro_full" | cut -d'-' -f1)
    local version
    version=$(echo "$distro_full" | cut -d'-' -f2)

    if [ "$distro" == "ubuntu" ]; then
        if [ "$version" == "18.04" ] && [ "$architecture" == "amd64" ]; then
            package_url="https://github.com/microsoft/do-client/releases/download/v1.0.0/ubuntu1804_x64-packages.tar"
            package_filename="ubuntu18_x64-packages.tar"
        elif [ "$version" == "18.04" ] && [ "$architecture" == "arm64" ]; then
            package_url="https://github.com/microsoft/do-client/releases/download/v1.0.0/ubuntu1804_arm64-packages.tar"
            package_filename="ubuntu1804_arm64-packages.tar"
        elif [ "$version" == "20.04" ] && [ "$architecture" == "amd64" ]; then
            package_url="https://github.com/microsoft/do-client/releases/download/v1.0.0/ubuntu2004_x64-packages.tar"
            package_filename="ubuntu20_x64-packages.tar"
        elif [ "$version" == "20.04" ] && [ "$architecture" == "arm64" ]; then
            package_url="https://github.com/microsoft/do-client/releases/download/v1.0.0/ubuntu2004_arm64-packages.tar"
            package_filename="ubuntu20_arm64-packages.tar"
        elif [ "$version" == "22.04" ] && [ "$architecture" == "amd64" ]; then
            package_url="https://github.com/microsoft/do-client/releases/download/v1.1.0/ubuntu2204_x64-packages.tar"
            package_filename="ubuntu2204_x64-packages.tar"
        elif [ "$version" == "22.04" ] && [ "$architecture" == "arm64" ]; then
            package_url="https://github.com/microsoft/do-client/releases/download/v1.1.0/ubuntu2204_arm64-packages.tar"
            package_filename="ubuntu2204_arm64-packages.tar"
        fi
    elif [ "$distro" == "debian" ] && [ "$version" == "10" ] && [ "$architecture" == "amd64" ]; then
        package_url="https://github.com/microsoft/do-client/releases/download/v1.0.0/debian10_x64-packages.tar"
        package_filename="debian10_x64-packages.tar"
    fi

    if [ -n "$package_url" ] && [ -n "$package_filename" ]; then
        wget "$package_url" -O "$package_filename"
        tar -xf "$package_filename"
        sudo apt-get install -y ./deliveryoptimization-agent_1.0.0_"$architecture".deb ./deliveryoptimization-plugin-apt_0.5.1_"$architecture".deb ./libdeliveryoptimization_1.0.0_"$architecture".deb
    else
        echo "Unsupported distro, version or architecture"
        exit 1
    fi
}

function extract_adu_srcs() {
    mkdir ~/adu_srcs/

    tar -xf ./testsetup/adu_srcs_repo.tar.gz -C ~/adu_srcs/
}
function register_extensions() {

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
    status_output=$(systemctl status deviceupdate-agent.service)

    main_pid=$(echo "$status_output" | awk '/Main PID/ {print $3}')

    if [ -d "/proc/$main_pid" ]; then
        process_user=$(stat -c %u "/proc/$main_pid")
        process_group=$(stat -c %g "/proc/$main_pid")

        if [ "$process_user" -eq "$adu_uid" ] && [ "$process_group" -eq "$adu_gid" ]; then
            echo "User and group for AducIotAgent service are adu:adu."
        else
            echo "User and group for AducIotAgent service are not adu:adu."
            exit 1
        fi
    else
        echo "Process with PID $main_pid does not exist."
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

extract_adu_srcs

configure_apt_repository "$distro" "$architecture"

#
# This MUST be run before we configure ais since AIS requires the keys and certs to be in their normal location
#
install_certs_and_keys

install_and_configure_ais

#
# Install the Device Update Artifact Under Test
#
if [[ $self_upgrade == "true" ]]; then
    sudo apt-get install -y deviceupdate-agent
    chmod u+x ./testsetup/apt_repo_setup.sh
    sudo ./testsetup/apt_repo_setup.sh -d ./testsetup/
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

install_do "$distro" "$architecture"

register_extensions

#
# Restart the deviceupdate-agent.service
#
# Note: We expect that everything should be setup for the deviceupdate agent at this point. Once
# we restart the agent we expect it to be able to boot back up and connect to the IotHub. Otherwise
# this test will be considered a failure.
sudo systemctl restart deviceupdate-agent.service

# verify_user_group_permissions

# verify_log_files

# test_shutdown_service

# sudo systemctl restart deviceupdate-agent.service
