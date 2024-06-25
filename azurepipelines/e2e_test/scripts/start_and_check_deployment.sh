#!/bin/bash

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
# Simple Test Script for initiating a deployment and checking the status of the deployment
# This script is intended to be run in an Azure DevOps pipeline
# The script will attempt to start a deployment and check the status of the deployment
# The script will return 0 if the deployment is successful and 1 if the deployment fails
export AZURE_CORE_NO_WARN=true

az config set extension.use_dynamic_install=yes_without_prompt --only-show-errors 2> /dev/null

sudo apt-get install -y jq

account_name=""
instance_name=""
group_id=""
deployment_id=""

update_name=""
update_provider=""
update_version=""

while [[ $1 != "" ]]; do
    case $1 in
    -g | --group-id)
        shift
        group_id=$1
        ;;
    -d | --deployment-id)
        shift
        deployment_id=$1
        ;;
    -a | --account-name)
        shift
        account_name=$1
        ;;
    -i | --instance-name)
        shift
        instance_name=$1
        ;;
    -n | --update-name)
        shift
        update_name=$1
        ;;
    -p | --update-provider)
        shift
        update_provider=$1
        ;;
    -v | --update-version)
        shift
        update_version=$1
        ;;
    *)
        echo "Invalid argument: $*"
        print_help
        exit 1
        ;;
    esac
    shift
done

if [[ -z $account_name || -z $instance_name || -z $group_id || -z $deployment_id ]]; then
    echo "Missing required device and account arguments"
    exit 1
fi

if [[ -z $update_name || -z $update_provider || -z $update_version ]]; then
    echo "Missing required update arguments"
    exit 1
fi

function add_module_twin_tag() {

    az iot hub module-twin update --device-id "ubuntu-2004-x509-test-device" --module-id "IoTHubDeviceUpdate" --hub-name "test-automation-iothub" --set "tags.ADUGroup=$group_id" 2> /dev/null || exit 1

}

function start_and_query_deployment() {
    # Attempt to create the deployment

    echo "Starting deployment"
    az iot du device deployment create --account "$account_name" --instance "$instance_name" --deployment-id "$deployment_id" --group-id "$group_id" --update-name "$update_name" --update-provider "$update_provider" --update-version "$update_version" 2> /dev/null || exit 1

    echo "Deployment started. Sleeping for 301 seconds to allow application"
    # Wait for 301 seconds to allow the message to propogate
    sleep 301

    echo "Checking for status of deployment"

    # Loop ten times checking the status every minute
    n=0
    while [[ $n != "10" ]]; do
        deployment_json=$(az iot du device deployment show --account "$account_name" --instance "$instance_name" --deployment-id "$deployment_id" --group-id "$group_id" --status)

        total_devices=$(echo "$deployment_json" | jq '.subgroupStatus[].totalDevices')
        succeeded_devices=$(echo "$deployment_json" | jq '.subgroupStatus[].devicesCompletedSucceededCount')
        failed_devices=$(echo "$deployment_json" | jq '.subgroupStatus[].devicesCompletedFailedCount')

        if [[ $total_devices == "0" ]]; then
            echo "No devices added yet. Waiting an extra bit to see if they'll figure it out."
            sleep 201
        elif [[ $total_devices == "$succeeded_devices" ]]; then
            echo "Deployment succeeded"
            return 0
        elif [[ $total_devices == "$failed_devices" ]]; then
            echo "Deployment failed"
            return 1
        else
            echo "Deployment status is $deployment_json"
            sleep 60
        fi
        n=$((n + 1))
    done

    echo "Deployment did not complete in time"

    return 1
}

add_module_twin_tag

status=1
start_and_query_deployment
status=$?

echo "Deleting the deployment to clean up..."
az iot du device deployment delete --account "$account_name" --instance "$instance_name" --deployment-id "$deployment_id" --group-id "$group_id" --yes 2> /dev/null || exit 1

if [[ $status -eq 0 ]]; then
    echo "Success!"
    exit 0
fi

echo "Failure" >&2
exit 1
