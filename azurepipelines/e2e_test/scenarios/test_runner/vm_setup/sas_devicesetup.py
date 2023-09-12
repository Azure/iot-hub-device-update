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
# python ./scenarios/test_runner/<test-script-name>.py
sys.path.append('./scenarios/test_runner/')
from scenario_definitions import DuScenarioDefinitionManager

def main():
    duTestConfig = DuAutomatedTestConfigurationManager.FromOSEnvironment()
    #
    # Instantiate the Wrapper Class from the config manager
    #
    duTestWrapper = duTestConfig.CreateDeviceUpdateTestHelper()

    aduScenarioDefinition = DuScenarioDefinitionManager.FromOSEnvironment()

    test_device_id = aduScenarioDefinition.test_device_id

    #
    # Delete the device if it exists, we don't care if we fail here.
    #
    duTestWrapper.DeleteDevice(test_device_id)
    #
    # Create the Device
    #
    connectionString = duTestWrapper.CreateSaSDevice(
        test_device_id, isIotEdge=True)

    if (len(connectionString) == 0):
        print_error("Failed to create the device in the IotHub")
        sys.exit(1)

    #
    # Create the du-config-ais.json JSON Object
    #
    duConfigAISJson = {
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
                                "connectionType": "AIS",
                                "connectionData": ""
                            },
                            "manufacturer": "contoso",
                            "model": "virtual-vacuum-v2"
                        }
        ]
    }

    #
    # Write the DU configuration out to disk so we can install it as a part of the test
    #

    with open('du-config-ais.json', 'w',encoding="utf-8") as jsonFile:
        configJson = json.dumps(duConfigAISJson)
        jsonFile.write(configJson)

    #
    # Create the du-config-connectionstr.json JSON Object
    #
    duConfigConnectionStrJson = {
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

    with open('du-config-connectionstr.json', 'w',encoding="utf-8") as jsonFile:
        configJson = json.dumps(duConfigConnectionStrJson)
        jsonFile.write(configJson)

    #
    # Create the config.toml contents
    #
    configTomlInfo = """\
[provisioning]
source = "manual"
connection_string = "{connectionString}"
[aziot_keys]
[preloaded_keys]
[cert_issuance]
[preloaded_certs]
""".format(connectionString=connectionString)

    with open('config.toml', 'w') as configToml:
        configToml.write(configTomlInfo)



if __name__ == '__main__':
    main()

def print_error(msg):
    print(msg, file=sys.stderr)

#
# Output is now a newly created device and a du-config.json file that will work with it
#
