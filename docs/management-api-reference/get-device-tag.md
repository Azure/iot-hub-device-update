# Get Device Tag

Gets details for an ADUGroup device tag

## HTTP Request

```http
GET https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v2/management/devicetags/{tagName}

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

## Responses

|   HTTP Code   |   Description  |
| :--------- | :---- |
|  200 OK | The device tag details |

```json
{
    "tagName": "group1",
    "deviceCount": 100
}
```
