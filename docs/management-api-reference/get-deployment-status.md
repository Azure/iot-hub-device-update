# Get Deployment Status

Get the status of a deployment

## HTTP Request

```http
GET https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v2/management/deployments/{deploymentId}/status
```

## URI Parameters

Name|Input|Required|Type|Description
----|------|------|------|------|
Account| Hostname|True|String|ADU Account Name
Instance| Hostname|True|String|ADU Instance Name
deploymentId | path | True | string | The deploymentId returned by the Get Deployment API

## Request Headers

Name|Description
----|------|
Authorization| Bearer.Required
Content-Type | application/json

## Responses

|   HTTP Code   |   Description  |
| :--------- | :---- |
|  200 OK | The status of the deployment |
|  404 NOT FOUND | Deployment not found |

```json
{
  "deploymentState": enum,
  "TotalDevices": 0,
  "DevicesIncompatibleCount": 0,
  "DevicesAlreadyInDeploymentCount": 0,
  "DevicesInProgressCount": 0,
  "DevicesCompletedFailedCount": 0,
  "DevicesCompletedSucceededCount": 0,
  "DevicesCanceledCount": 0
}
```

## Deployment State Type enum

Name|Description
----|------|
Active | Deployment is Active and not Canceled
Canceled | Deployment has been Canceled
