# Get Group Details

Gets details for an ADU Device Group

## HTTP Request

```http
GET https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v2/management/groups/{groupId}

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

## Responses

|   HTTP Code   |   Description  |
| :--------- | :---- |
|  200 OK | The group details |

```json
{
    "groupId": "mygroup",
    "tags": [ "mygroup" ],
    "createdDateTime": "2020-10-24T00:32:09.7197303+00:00",
    "groupType": "IoTHubTag",
    "deviceCount": 10
}
```
