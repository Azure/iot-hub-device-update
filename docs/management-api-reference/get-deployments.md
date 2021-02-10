# Get Deployments

Get the list of all deployments that have been created in this instance

## HTTP Request

```http
GET https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v2/management/deployments
```

## Request Headers

Name|Description
----|------|
Authorization| Bearer.Required
Content-Type | application/json

## Responses

|   HTTP Code   |   Description  |
| :--------- | :---- |
|  200 OK | List of all deployments in this instance |

```json
{
    "value": [
        {
            "deploymentId": "testdeploymentid1",
            "deploymentType": "Complete",
            "deviceClassId": "c94e3c87fbf98063c20c3269f1c9e58d255906dd",
            "startDateTime": "2019-09-01T16:29:56.5770502+00:00",
            "deviceGroupType": "Devices",
            "deviceGroupDefinition": [
                "test-device-1"
            ],
            "updateId": {
                "provider": "contoso",
                "name": "virtual-machine",
                "version": "3.0"
            },
            "isCanceled": true,
            "isCompleted": false,
            "isRetry": false
        },
        {
            "deploymentId": "e20dc00f-879a-46b0-a544-b1d1f64da83c",
            "deploymentType": "Complete",
            "deviceClassId": "abce3c87fbf98063c20c3269f1c9e58d255907ee",
            "startDateTime": "2019-09-01T16:29:56.5770502+00:00",
            "deviceGroupType": "Devices",
            "deviceGroupDefinition": [
                "test-device-2"
            ],
            "updateId": {
                "provider": "fabrikam",
                "name": "thermometer",
                "version": "10.0"
            },
            "isCanceled": false,
            "isCompleted": false,
            "isRetry": false
        }
    ]
}
```
