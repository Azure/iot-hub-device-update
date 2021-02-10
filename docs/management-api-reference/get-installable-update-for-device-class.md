# Get Installable Update for a Device Class

Gets a list of updates in the ADU instance that are compatible with the given device class.

## HTTP Request

```http
GET https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v2/management/deviceclasses/{deviceClassId}/installableupdates
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

|   HTTP Code   |   Description  |
| :--------- | :---- |
|  200 OK | List of updates compatible with the device class |
|  404 NOT FOUND | Device class not found |

Name|Type|Description
----|------|------|
Provider| string|Provider part of the update identity
Name| string|Name part of the update identity
Version| version | Version part of the update identity

```json
[
  {
    "provider": "myProvider1",
    "name": "name",
    "version": "1.2.3.4"
  },
  {
    "provider": "myProvider2",
    "name": "name2",
    "version": "2.3.4.5"
  }
]
```
