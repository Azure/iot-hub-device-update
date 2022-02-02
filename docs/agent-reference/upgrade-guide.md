# DU Agent Upgrade Guide

## Configuration File Changes

|| Public Preview | Public Preview Refresh|
|----|----|----|
| File name| /etc/adu/adu-conf.txt | /etc/adu/du-config.json |
| File name (Yocto Reference Image)| /adu/adu-conf.txt | /adu/du-config.json |
| File format| Text | JSON document|

### DU Agent Settings Changes

The DU Agent properties can be specified in the `agents` array. Note that for Public Preview Refresh, all agent properties will be read from `agents[0]`.

<table>
<tr>
<td>DU Agent Properties</td><td>Public Preview</td><td>Public Preview Refresh</td></tr>

<tr>
<td>
Manufacturer and Model
</td>

<td>

```text
...

aduc_manufacturer=<device manufacturer name>
aduc_model=<device model name>

...
```

<td>

The `manufacturer` and `model` must be specified in an `agents[0].manufacturer` and `agents[0].model`, respectively.

For example:

```json
{

...

    "agents": [
        {
            "name": "...",
            "runas": "...",
            "connectionSource": { ... },
            "manufacturer": "<device manufacturer name>",
            "model": "<device model name>"
        }
    ]

...

}
```
</td>

</td></tr>
<tr>
<td>IoT Hub Connection
</td>

<td>

```text
...

connection_string=<device connection string>

...
```

<td>

```json
{

...

    "agents": [
        {
            "name": "<agent process name>",
            "runas": "adu",
            "connectionSource": {
                "connectionType": "string",
                "connectionData": "<device or module connection string>"
            },
            "manufacturer": "...",
            "model": "..."
        }
    ]

...

}
```

</td>

</td></tr>
<tr><td></td><td></td><td></td></tr>
</table>

## Install Device Update Agent Package from packages.microsoft.com

> **Prerequisite** | Ensure that the device can access the Microsoft installation packages by following [this instruction](https://docs.microsoft.com/azure/iot-edge/how-to-provision-single-device-linux-symmetric?view=iotedge-2020-11&preserve-view=true&tabs=azure-portal#access-the-microsoft-installation-packages)

For a device that previously installed the DU Agent Public Preview version from packages.microsoft.com, you can choose from following upgrade options:

### Option 1 - Manually install debian package

You can manually install a new version of `deviceupdate-agent` debian package from packages.microsoft.com (for supported distros and architectures) by remotely running following command on the device:

```sh
sudo apt-get purge adu-agent #In case you have an old adu-agent in the system
sudo apt-get purge deviceupdate-agent
sudo apt-get install deviceupdate-agent
```

>**Note** | Please mind potential data loss when you use `purge`. If needed, save a copy of your configuration file, log files, etc.

### Option 2 - Deploy an APT update using ADU Service

To upgrade devices that currently connected to IoT Hub using Device Update Service Deployment (APT Update)

#### 1. Create an APT Update Manifest using this template

> **Note** | replace `name` and `version` accordingly.

````json
{
    "name": "<update name>",
    "version": "<update version>",
    "packages": [
        {
            "name": "deviceupdate-agent",
        }
    ]
}
````

#### 2. Import the update to ADU service

See [this document](../../tools/AduCmdlets/README.md) for how to import an update to ADU service.

>**Note**
>1. Specify `provider`, `name`, and `version` as appropriate.
>2. The DU Agent Public Preview version only support an Update Manifest version 2. The Update Manifest version must be specified correctly when importing the update.

#### 3. Deploy the update

Follow [Deploy A Device Update Guide](https://docs.microsoft.com/en-us/azure/iot-hub-device-update/deploy-update) to deploy the update imported in above step to a desired group of device.

Once the target device(s) installed and applied the update, the DU Agent on the device should automatically restart and reconnect to the IoT Hub.

## Post-upgrade Step

### Update the ADUGroup property in Device/Module Twin

Since the new DU Agent using newer version of IoT Hub PnP (Device Update) Interface, every device with upgraded DU Agent will be removed from previous Device Group, and must be re-assigned to a new Device Group.

See [Create Update Group](https://docs.microsoft.com/en-us/azure/iot-hub-device-update/create-update-group) for more details.

### Fill in du-config.json

Option 1:
Manually fill in the configuration file with the instructions in **du-config.json** file. Note that values like `aduShellTrustedUsers` and `runas` are default values, you can change them as needed.

Option 2:
If you have an existing saved copy of **adu-conf.txt** file, You can run this script [config-integration.sh](../../scripts/config-integration.sh) to migrate from text format to the new **du-config.json** file in json format.

```sh
. scripts/config-integration.sh [copy of adu-conf.txt file]
```

Please check the **du-config.json** file after calling the script, to ensure all mandatory fields are filled in.
