# Get Groups

Gets a list of ADU groups

## HTTP Request

```http
GET https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v2/management/groups

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
|  200 OK | The group details |

```json
{
    "value": [
        {
            "groupId": "mygroup",
            "tags": [ "mygroup" ],
            "createdDateTime": "2020-10-24T00:32:09.7197303+00:00",
            "groupType": "IoTHubTag",
            "deviceCount": 10
        },
        {
            "groupId": "mygroup2",
            "tags": [ "mygroup2" ],
            "createdDateTime": "2020-11-24T00:32:09.7197303+00:00",
            "groupType": "IoTHubTag",
            "deviceCount": 100
        }
    ]
}
```
