# Create or Update Deployment

Create or update a Deployment

## HTTP Request

```http
PUT https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v2/management/deployments/{deploymentId}
```

## Deployment Resource Object

Name| Type | Description |Required
----|------|------|---------
deploymentId | String | The name of your deployment that must be unique | True
deploymentType | Enum | The supported type is "complete" | True
deviceClassId | String | This is returned from the get device classes API, must not be included if deploying to a device group | True
startDateTime | Long Time | The start time requires ISO-8601 format | True
deviceGroupType| Enum | The DeviceGroupType enum  |True
deviceGroupDefinition| String | In the case of deviceGroupType = all, this must be empty. For devices, it is the list of deviceIds for deviceGroupDefinitions it is the list of group Ids | True
updateId | Object | The update resource type | True

```json

{
  "deploymentId": "myDeploymentId",
  "deploymentType": "complete",
  "deviceClassId": "c83e3c87fbf98074c20c3269f1c9e58d255906dd",
  "startDateTime": "2020-05-27T19:25:57.6448039Z",
  "deviceGroupType": "devices",
  "deviceGroupDefinition": [
  	"myDevice1",
    "myDevice2"
  	],
  "updateId": {
    "provider": "contoso",
    "name": "virtual-machine",
    "version": "3.0"
  }
}
```

## Deployment Group Type enum

Name| Description|
----|------
All | Specify all devices in device class
Devices | The exact list of devices specified in the device group list must at least have 1 and not exceed 100
DeviceGroupDefinitions | Group Ids defined in ADU must number between 1 and not exceed 20

## Update Id

Name| Type | Description| Required
----|------|------|------
provider | string |The provider defined when update is imported | True
name | string | The name of the update defined when imported | True
version | string |The version of the update defined when imported | True

## Request Headers

Name|Description
----|------|
Authorization| Bearer.Required
Content-Type | application/json

## Responses

|   HTTP Code   |   Description  |
| :--------- | :---- |
|  200 OK | Deployment was created or updated successfully (synchronously).|
|  400 BAD REQUEST | Returned if endDateTime is not at least 1 hour later than startDateTime, or if any of the provided values in the request are invalid data.|
|  404 NOT FOUND | Deployment not found |

```json
{
    "deploymentId": "myDeploymentId",
    "deploymentType": "Complete",
    "deviceClassId": "c83e3c87fbf98074c20c3269f1c9e58d255906dd",
    "startDateTime": "2020-05-27T19:25:57.6448039+00:00",
    "deviceGroupType": "Devices",
    "deviceGroupDefinition": [
        "myDevice1",
        "myDevice2"
    ],
    "updateId": {
        "provider": "contoso",
        "name": "virtual-machine",
        "version": "3.0"
    },
    "isCanceled": false,
    "isCompleted": false,
    "isRetry": false
}
```
