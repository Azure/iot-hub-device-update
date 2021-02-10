# Get Available Device Tags

Gets the list of ADUGroup IoT Hub tags that are available to use to create new device groups

## HTTP Request

```http
GET https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v2/management/devicetags

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
|  200 OK | The list of available device tags |

```json
{
    "value": [
        {
            "tagName": "group1",
            "deviceCount": 100
        },
        {
            "tagName": "group2",
            "deviceCount": 200
        }
    ]
}
```
