#! /bin/sh

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

demo_dir=/usr/local/demo

# Ensure the demo directory exists
if [ ! -d "$demo_dir" ]; then
    echo "Demo directory $demo_dir does not exist. Please create it first."
    exit 1
fi

demo_devices_dir="$demo_dir/demo-devices/contoso-devices"

# Ensure the demo devices directory exists
if [ ! -d "$demo_devices_dir" ]; then
    echo "Demo devices directory $demo_devices_dir does not exist. Please create it first."
    exit 1
fi

target_devices_dir=/usr/local/contoso-devices
# Ensure the target devices directory exists
if [ ! -d "$target_devices_dir" ]; then
    echo "Target devices directory $target_devices_dir does not exist. Please create it first."
    exit 1
fi

# Show demo_dir, demo_devices_dir, and target_devices_dir
printf "Demo root dir: %s\n" $demo_dir
printf "Virtual devices root dir: %s\n" $demo_devices_dir
printf "Target devices dir: %s\n" $target_devices_dir

printf "\n - Create a virtual device 'contoso vacuum-1' for testing purposes..."
sudo rm -f -r "$target_devices_dir"
sudo mkdir -p "$target_devices_dir"

sudo cp -r $demo_devices_dir/* "$target_devices_dir"
sudo chmod 555 -R "$target_devices_dir"

printf "\n - Reset installed-criteria data..."
sudo rm -f -r /var/lib/adu/installedcriteria
printf "\n\nDone\n"
