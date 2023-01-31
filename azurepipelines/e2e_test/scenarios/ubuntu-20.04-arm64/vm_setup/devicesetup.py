# -------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See License.txt in the project root for
# license information.
# --------------------------------------------------------------------------
import json
import sys

# Note: the intention is that this script is called like:
# python ./<scenario-name>/vm-setup/devicesetup.py
sys.path.append('./scenarios/')
from testingtoolkit import DeviceUpdateTestHelper,DuAutomatedTestConfigurationManager

# Note: the intention is that this script is called like:
# python ./scenarios/<scenario-name>/<test-script-name>.py
sys.path.append('./scenarios/ubuntu-20.04-amd64/')
from scenario_definitions import test_device_id


def main():
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
    connectionString = duTestWrapper.CreateDevice(test_device_id, isIotEdge=True)

    if (len(connectionString) == 0):
        print_error("Failed to create the device in the IotHub")
        sys.exit(1)

    #
    # Create the du-config.json JSON Object
    #
    duConfigJson = {
                    "schemaVersion": "1.1",
                    "aduShellTrustedUsers": [
                        "adu",
                        "do"
                    ],
                    "iotHubProtocol": "mqtt",
                    "manufacturer": "contoso",
                    "model": "virtual-vacuum-v2",
                    "agents": [
                        {
                        "name": "main",
                        "runas": "adu",
                        "connectionSource": {
                            "connectionType": "string",
                            "connectionData": connectionString
                        },
                        "manufacturer": "contoso",
                        "model": "virtual-vacuum-v2"
                        }
                    ]
                    }

    #
    # Write the configuration out to disk so we can install it as a part of the test
    #

    with open('du-config.json', 'w',encoding="utf-8") as jsonFile:
        configJson = json.dumps(duConfigJson)
        jsonFile.write(configJson)


if __name__ == '__main__':
    main()

def print_error(msg):
    print(msg, file=sys.stderr)

#
# Output is now a newly created device and a du-config.json file that will work with it
#
