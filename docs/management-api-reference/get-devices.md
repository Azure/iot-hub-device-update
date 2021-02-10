# Get Devices

Gets the list of devices being managed by the ADU instance.

## HTTP Request

```http
GET https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v2/management/devices
```

## URI Parameters

Name|Input|Required|Type|Description
----|------|------|------|------|
Account| Hostname|True|String|ADU Account Name
Instance | Hostname|True|String|ADU Instance Name

## Request Headers

Name|Description
----|------|
Authorization| Bearer.Required

## Responses

|   HTTP Code   |   Description  |
| :--------- | :---- |
|  200 OK | List of device classes in the instance |

```json
{
    "value": [
        {
            "deviceId": "test-device-1",
            "deviceClassId": "f38bc5237d50fee933b11146d6804f3acdd605dd",
            "manufacturer": "Contoso",
            "model": "testmodel1",
            "lastAttemptedUpdateId": {
                "provider": "Contoso",
                "name": "testmodel1",
                "version": "1.0.1354.1"
            },
            "installedUpdateId": {
                "provider": "Contoso",
                "name": "testmodel1",
                "version": "1.0.1354.1"
            },
            "onLatestUpdate": false,
            "deploymentStatus": "Failed",
            "groupId": null,
            "lastDeploymentId": "testdeploymentid1"
        },
        {
            "deviceId": "test-device-2",
            "deviceClassId": "d08c9c774a1c541ecc88e8c65b6431fd8b7eafc3",
            "manufacturer": "Microsoft-Corporation",
            "model": "Virtual-Machine",
            "lastAttemptedUpdateId": null,
            "installedUpdateId": null,
            "onLatestUpdate": false,
            "deploymentStatus": "InProgress",
            "groupId": "groupname1",
            "lastDeploymentId": "testdeploymentid2"
        },
        {
            "deviceId": "test-device-3",
            "deviceClassId": "d08c9c774a1c541ecc88e8c65b6431fd8b7eafc3",
            "manufacturer": "microsoft-corporation",
            "model": "virtual-machine",
            "lastAttemptedUpdateId": {
                "provider": "microsoft-corporation",
                "name": "virtual-machine",
                "version": "111.0"
            },
            "installedUpdateId": {
                "provider": "microsoft-corporation",
                "name": "virtual-machine",
                "version": "111.0"
            },
            "onLatestUpdate": true,
            "deploymentStatus": "Succeeded",
            "groupId": null,
            "lastDeploymentId": "testdeploymentid3"
        }
    ]
}
```
