# Get Deployment Details

Get the details of a deployment when it was created

## HTTP Request

```http
GET https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v2/management/deployments/{deploymentId}
```

## Request Headers

Name|Description
----|------|
Authorization| Bearer.Required
Content-Type | application/json

## Responses

|   HTTP Code   |   Description  |
| :--------- | :---- |
|  200 OK | The details of the deployment |
|  404 NOT FOUND | Deployment not found |

```json
{
  "deploymentId": "myDeploymentId",
  "deviceClassId": "c83e3c87fbf98074c20c3269f1c9e58d255906dd",
  "deploymentType": "Complete",
  "startDateTime": "2019-07-12T16:29:56.5770502Z",
  "deviceGroupType": "Devices",
  "deviceGroupDefinition": [
    "myDevice1",
    "myDevice2"
  ],
  "updateId": {
    "provider": "myProvider",
    "name": "myName",
    "version": "1.2.3.4"
  },
  "isCanceled": true,
  "isCompleted": false,
  "isRetry": false
}
```
