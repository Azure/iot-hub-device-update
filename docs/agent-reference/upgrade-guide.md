# DU Agent Upgrade Guide [Draft]

## Things To Know

Based on feedback from Public Preview customers, many changes and improvements has been made to the Device Update Agent, including:

### Configuration File Changes

|| Pubic Preview | Public Preview Refresh|
|----|----|----|
| File name| /etc/adu/adu-conf.txt | /etc/adu/du-config.json |
| File name (Yocto Reference Image)| /adu/adu-conf.txt | /adu/du-config.json |
| File format| Text | JSON document|

### DU Agent Settings Changes

The DU Agent properties can be specified in the `agents` array. Note that for Public Preview Refresh, only 1st Agent Settings is supported.  

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

The `manufacturer` and `model` must be specified in an `agents[0].manufacturer` and `agent[0].model` respectively.  

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
                "connectionData": "<devcie or modeule connection string>"
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

For a device that previously installed the DU Agent Public Preview version from pakcages.microsoft.com, you can choose from following upgrade options:

### Option 1 - Manually install debian package

You can manually install a new version of `deviceupdate-agent` debian package from packags.microsoft.com (for supported distros and architectures) by remotely executing following command on the device:

```sh
sudo apt-get install deviceupdate-agent
```

>**Note** | Data in existing **adu-conf.txt** file will be automatically migrated to the new **du-config.json** file.

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

>**Note**<br/> 1) Specify `provider`, `name`, and `version` as appropriate. <br/>2) The DU Agent Public Preview version only support an Update Manfest version 2. The Update Manifest Version must be specified correctly when importing the update.

#### 3. Deploy the update

Follow this [Deploy A Device Update Guide](https://docs.microsoft.com/en-us/azure/iot-hub-device-update/deploy-update) to deploy the update imported in above step to a desired group of device.  

Once the target device(s) installed and applied the update, the DU Agent on the device should automatically restart and reconnect to the IoT Hub.  

## Post-upgrade Step

### Update the ADUGroup property in Device/Module Twin

Since the new DU Agent using newer version of IoT Hub PnP (Device Update) Interface, every device with upgraded DU Agent will be removed from previous Device Group, and must be re-assigned to a new Device Group.  

See [Create Update Group](https://docs.microsoft.com/en-us/azure/iot-hub-device-update/create-update-group) for more details.
