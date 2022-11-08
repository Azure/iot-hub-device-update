#! /bin/sh

# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

demo_dir=~/demo
demo_devices_dir="$demo_dir/demo-devices/contoso-devices"
target_devices_dir=/usr/local/contoso-devices

printf "\n#\n# Create a virtual device 'contoso vacuum-1' for testing purposes...\n"
sudo rm -f -r "$target_devices_dir"
sudo mkdir -p "$target_devices_dir"

sudo cp -r $demo_devices_dir/* "$target_devices_dir"
sudo chmod 555 -R "$target_devices_dir"

printf "#\n# Reset installed-criteria data..."
sudo rm -f -r /var/lib/adu/installedcriteria

