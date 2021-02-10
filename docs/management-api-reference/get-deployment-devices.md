# Get Deployment Devices

Get devices in the deployment, including a subset of devices in a particular device state within the deployment.

## HTTP Request

```http
GET https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v1/management/deployments/{deploymentId}/devicelist
```

## URI Parameters

Name|Input|Required|Type|Description
----|------|------|------|------|
Account| Hostname|True|String|ADU Account Name
Instance| Hostname|True|String|ADU Instance Name
deploymentId | path | True | string | The deploymentId returned by the Get Deployment API
deviceState | path | False | string | The state of a device.

### deviceState query values

Name| Description
----|------
Incompatible | Incompatible device
NotStarted | Update has not started
InProgress | Update in progress
CompletedFailed | Deployment failed
CompletedSucceeded | Deployment succeeded
Canceled | Deployment Canceled
AlreadyInDeployment | Deployment being rolled out

## Request Headers

Name|Description
----|------|
Authorization| Bearer.Required

## Responses

|   HTTP Code   |   Description  |
| :--------- | :---- |
|  200 OK | The devices in the deployment |
|  404 NOT FOUND | Deployment not found |

```json
[
  "device1",
  "device2"
]
```
