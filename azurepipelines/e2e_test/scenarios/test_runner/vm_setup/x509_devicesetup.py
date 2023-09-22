# -------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See License.txt in the project root for
# license information.
# --------------------------------------------------------------------------
import argparse
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

def print_error(msg):
    print(msg, file=sys.stderr)

def read_thumbprint_from_file(thumbprint_file):
    thumbprint = ""
    with open(thumbprint_file, 'r') as thumbprintFile:
        thumbprint = thumbprintFile.read().replace('\n', '')
    return thumbprint

def main():
    parser = argparse.ArgumentParser(description='Create a device and generate a du-config.json file for it using x509 provisioning')
    parser.add_argument('-p','--primary-certificate-thumbprint', required=True, help='The primary certificate thumbprint to use for the device')
    parser.add_argument('-s','--secondary-certificate-thumbprint', required=True, help='The secondary certificate thumbprint to use for the device')
    parser.add_argument('-d','--device-id', required=True, help='The device id to use for the device')
    parser.add_argument('-k','--key-file-path', required=True, help='The path the public key will be placed on the device')
    parser.add_argument('-c','--certificate-file-path', required=True, help='The path to the certificate file that will be placed on the device')
    args = parser.parse_args()

    if (args.primary_certificate_thumbprint is None or args.secondary_certificate_thumbprint is None):
        print("There is a certificate thumbprint or device id missing from the argument list")
        sys.exit(1)

    if (args.key_file_path is None or args.certificate_file_path is None):
        print("The key file path or certificate file path is missing from the argument list")
        sys.exit(1)

    if (args.device_id is None):
        print("The device_id was not passed to the file")
        sys.exit(1)

    # given some thumbr
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

    primary_cert_thumb = read_thumbprint_from_file(args.primary_certificate_thumbprint)
    secondary_cert_thumb = read_thumbprint_from_file(args.secondary_certificate_thumbprint)

    if (primary_cert_thumb == "" or secondary_cert_thumb == ""):
        print_error("Failed to read the thumbprints from the files")
        sys.exit(1)
    #
    # Need to read the thumbprins out of the files
    #
    #
    # Create the Device
    #
    connectionString = duTestWrapper.Createx509Device(
        test_device_id, primary_cert_thumb, secondary_cert_thumb, isIotEdge=True)

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
iothub_hostname = "test-automation-iothub.azure-devices.net"
device_id = "{device_id}"

[provisioning.authentication]
method = "x509"
identity_pk = "file://{key_path}"
identity_cert = "file://{cert_path}"
""".format(device_id=args.device_id, key_path=args.key_file_path, cert_path=args.certificate_file_path)


    with open('config.toml', 'w') as configToml:
        configToml.write(configTomlInfo)



if __name__ == '__main__':
    main()


#
# Output is now a newly created device and a du-config.json file that will work with it
#
