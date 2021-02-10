# Get Device Classes for a Device

Gets a list of device classes that the given device belongs to.

## HTTP Request

```http
GET https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v1/management/devices/{deviceId}/deviceclasslist
```

## URI Parameters

Name|Input|Required|Type|Description
----|------|------|------|------|
Account| Hostname|True|String|ADU Account Name
Instance | Hostname|True|String|ADU Instance Name
deviceId | path | True | string | A device ID as returned by the Get Devices in Device Class API.

## Request Headers

Name|Description
----|------|
Authorization| Bearer.Required

## Responses

|   HTTP Code   |   Description  |
| :--------- | :---- |
|  200 OK | List of device classes that the device belongs to |

```json
[
  "deviceClassId1",
  "deviceClassId2"
]
