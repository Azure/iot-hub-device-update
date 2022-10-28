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
# Install the Microsoft APT repository
#

wget https://packages.microsoft.com/config/ubuntu/18.04/multiarch/packages-microsoft-prod.deb -O packages-microsoft-prod.deb
sudo dpkg -i packages-microsoft-prod.deb
rm packages-microsoft-prod.deb

#
# Update the apt repositories
#
sudo apt-get update

#
# Install Device Update Dependencies from APT
#
# Note: If there are other dependencies tht need to be installed via APT or other means they should
# be added here. You might be installing iotedge, another package to setup for a deployment test, or
# anything else.

# Handle installing DO from latest build instead of packages.microsoft.com
wget https://github.com/microsoft/do-client/releases/download/v1.0.0/ubuntu1804_x64-packages.tar -O ubuntu18_x64-packages.tar
tar -xvf ubuntu18_x64-packages.tar
sudo apt-get install -y ./deliveryoptimization-agent_1.0.0_amd64.deb ./deliveryoptimization-plugin-apt_0.5.1_amd64.deb ./libdeliveryoptimization_1.0.0_amd64.deb


#
# Install the Device Update Artifact Under Test
#
sudo apt-get install -y ./testsetup/deviceupdate-package.deb

#
# Install the du-config.json so the device can be provisioned
#
# Note: In other setup scripts there may be more info that needs to
# go here. For instance you might have the config.toml for IotEdge,
# another kind of diagnostics file, or other kinds of data
# this is the area where such things can be added
sudo cp ./testsetup/du-config.json /etc/adu/du-config.json


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


#
# Restart the deviceupdate-agent.service
#
# Note: We expect that everything should be setup for the deviceupdate agent at this point. Once
# we restart the agent we expect it to be able to boot back up and connect to the IotHub. Otherwise
# this test will be considered a failure.
sudo systemctl restart deviceupdate-agent.service
