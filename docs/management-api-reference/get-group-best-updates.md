# Get best updates for a group

Get the best updates for a group

## HTTP Request

```http
GET https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v2/management/groups/{groupId}/bestupdates
```

## URI Parameters

Name|Input|Required|Type|Description
----|------|------|------|------|
Account| Hostname|True|String|ADU Account Name
Instance| Hostname|True|String|ADU Instance Name
GroupId| Hostname|True|String|Group Id

## Request Headers

Name|Description
----|------|
Authorization| Bearer.Required
Content-Type | application/json

## Responses

|   HTTP Code   |   Description  |
| :--------- | :---- |
|  200 OK | The best updates for a group

```json
{
    "value": [
        {
            "UpdateId": {
                "provider": "Contoso",
                "name": "Virtual-Machine",
                "version": "16.0"
            },
            "deviceCount": 100
        },
        {
            "UpdateId": {
                "provider": "Fabrikam",
                "name": "thermometer",
                "version": "1.0"
            },
            "deviceCount": 200
        },
        {
            "UpdateId": {
                "provider": "Contoso",
                "name": "lamp",
                "version": "3.14"
            },
            "deviceCount": 50
        }
    ]
}
```
