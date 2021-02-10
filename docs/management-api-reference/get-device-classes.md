# Get Device Classes

Gets the list of device classes being managed by the ADU instance.

## HTTP Request

```http
GET https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v2/management/deviceclasses
```

## URI Parameters

Name|Input|Required|Type|Description
----|------|------|------|------|
Account| Hostname|True|String|ADU Account Name
Instance | Hostname|True|String|ADU Instance Name

## Request Headers

Name|Description
----|------|
Authorization| Bearer.Required

## Responses

|   HTTP Code   |   Description  |
| :--------- | :---- |
|  200 OK | List of device classes in the instance |

```json
{
    "value": [
        {
            "deviceClassId": "c83e3c87fbf98074c20c3269f1c9e58d255906dd",
            "manufacturer": "Contoso",
            "model": "Virtual-Machine",
            "bestCompatibleUpdateId": {
                "provider": "Contoso",
                "name": "Virtual-Machine",
                "version": "16.0"
            }
        },
        {
            "deviceClassId": "d20d7ecefcf9b64a9078aaed3a0e04d44c42779c",
            "manufacturer": "Contoso",
            "model": "Video"
        },
        {
            "deviceClassId": "8431f6d0c986d38a4162db561b7a494aec60c3e4",
            "manufacturer": "fabrikam",
            "model": "virtual-machine"
        }
    ]
}
```
