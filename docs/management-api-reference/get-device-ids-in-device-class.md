# Get Device Ids in Device Class

Gets the list of device ids in a device class.

## HTTP Request

```http
GET https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v2/management/deviceclasses/{deviceClassId}/deviceids
```

## URI Parameters

Name|Input|Required|Type|Description
----|------|------|------|------|
Account| Hostname|True|String|ADU Account Name
Instance| Hostname|True|String|ADU Instance Name
deviceClassId | path | True | string | A device class ID as returned by the Get Device Classes API

## Request Headers

Name|Description
----|------|
Authorization| Bearer.Required

## Responses

## Responses

|   HTTP Code   |   Description  |
| :--------- | :---- |
|  200 OK | List of devices in the device class |

```json
[
  "deviceId1",
  "deviceId2"
]
```
