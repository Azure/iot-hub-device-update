# Get Device

Gets the properties and last deployment status for a device

## HTTP Request

```http
GET https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v2/management/devices/{deviceId}
```

## URI Parameters

Name|Input|Required|Type|Description
----|------|------|------|------|
Account| Hostname|True|String|ADU Account Name
Instance| Hostname|True|String|ADU Instance Name
deviceId | path | True | string | The device ID

## Request Headers

Name|Description
----|------|
Authorization| Bearer.Required

## Responses

|   HTTP Code   |   Description  |
| :--------- | :---- |
|  200 OK | The properties and last deployment status for the device |

```json
{
    "deviceId": "myDeviceId",
    "deviceClassId": "c83e3c87fbf98074c20c3269f1c9e58d255906dd",
    "manufacturer": "Contoso",
    "model": "Virtual-Machine",
    "lastAttemptedUpdateId": {
        "provider": "contoso",
        "name": "virtual-machine",
        "version": "3.0"
    },
    "installedUpdateId": {
        "provider": "contoso",
        "name": "virtual-machine",
        "version": "3.0"
    },
    "onLatestUpdate": false,
    "deploymentStatus": "Succeeded",
    "groupId": "myGroupId",
    "lastDeploymentId": "myDeploymentId"
}
```
