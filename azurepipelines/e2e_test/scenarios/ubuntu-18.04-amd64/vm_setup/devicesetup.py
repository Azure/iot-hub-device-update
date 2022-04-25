# -------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See License.txt in the project root for
# license information.
# --------------------------------------------------------------------------
import json
import os
import sys

# Note: the intention is that this script is called like:
# python ./<scenario-name>/vm-setup/devicesetup.py
sys.path.append('./scenarios/')
sys.path.append('./scenarios/ubuntu-18.04-amd64/')
print(os.path.abspath('./'))
from scenario_definitions import test_device_id
from testingtoolkit import DeviceUpdateTestHelper,DuAutomatedTestConfigurationManager

duTestConfig = DuAutomatedTestConfigurationManager.FromOSEnvironment()


#
# Instantiate the Wrapper Class from the config manager
#
duTestWrapper = duTestConfig.CreateDeviceUpdateTestHelper()

#
# Delete the device if it exists, we don't care if we fail here.
#
duTestWrapper.DeleteDevice(test_device_id)

#
# Create the Device
#
connectionString = duTestWrapper.CreateDevice(test_device_id,isIotEdge=True)

if (len(connectionString) == 0):
    print("Failed to create the device in the IotHub")
    exit(1)

#
# Create the du-config.json JSON Object
#

duConfigJson = {
                "schemaVersion": "1.0",
                "aduShellTrustedUsers": [
                    "adu",
                    "do"
                ],
                "manufacturer": "Fabrikaam",
                "model": "Vacuum",
                "agents": [
                    {
                    "name": "main",
                    "runas": "adu",
                    "connectionSource": {
                        "connectionType": "string",
                        "connectionData": connectionString
                    },
                    "manufacturer": "Fabrikaam",
                    "model": "Vacuum"
                    }
                ]
                }

#
# Write the configuration out to disk so we can install it as a part of the test
#

with open('du-config.json','w') as jsonFile:
    configJson = json.dumps(duConfigJson)
    jsonFile.write(configJson)

#
# Output is now a newly created device and a du-config.json file that will work with it
#
