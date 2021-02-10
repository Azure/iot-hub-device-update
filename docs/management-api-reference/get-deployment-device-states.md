# Get Deployment Device States

Get devices in the deployment along with their deployment state.

## HTTP Request

```http
GET https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v2/management/deployments/{deploymentId}/devicestates
```

## URI Parameters

Name|Input|Required|Type|Description
----|------|------|------|------|
Account| Hostname|True|String|ADU Account Name
Instance| Hostname|True|String|ADU Instance Name
deploymentId | path | True | string | The deploymentId returned by the Get Deployment API

### deviceState query values

Name| Description
----|------
Incompatible | Incompatible device
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
{
    "value": [
        {
            "DeviceId": "testdevice1",
            "DeviceState": enum,
            "RetryCount": 1,
            "MovedOnToNewDeployment": "false"
        },
        {
            "DeviceId": "testdevice2",
            "DeviceState": enum,
            "RetryCount": 1,
            "MovedOnToNewDeployment": "true"
        },
        {
            "DeviceId": "testdevice3",
            "DeviceState": enum,
            "RetryCount": 3,
            "MovedOnToNewDeployment": "false"
        }
    ]
}
```

## Device State Type enum

Name|Description
----|------|
Incompatible | Device is incompatible with the update in the deployment
AlreadyInDeployment | Device is not receiving the deployment because it is already in another deployment
InProgress | Device is currently in progress on the deployment
CompletedFailed | Device has completed the deployment but failed
CompletedSucceeded | Device has completed the deployment and succeeded
Canceled | Device has been canceled from this deployment
