# Get Update Compliance for all devices

Get the update compliance status for all devices

## HTTP Request

```http
GET https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v2/management/updatecompliance
```

## URI Parameters

Name|Input|Required|Type|Description
----|------|------|------|------|
Account| Hostname|True|String|ADU Account Name
Instance| Hostname|True|String|ADU Instance Name

## Request Headers

Name|Description
----|------|
Authorization| Bearer.Required
Content-Type | application/json

## Responses

|   HTTP Code   |   Description  |
| :--------- | :---- |
|  200 OK | The update compliance status of all devices

```json
{
    "totalDeviceCount": 5,
    "onLatestUpdateDeviceCount": 3,
    "newUpdatesAvailableDeviceCount": 1,
    "updatesInProgressDeviceCount": 1
}
```
