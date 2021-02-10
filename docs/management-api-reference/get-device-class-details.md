# Get Device Class Details

Gets the details of a device class.

## HTTP Request

```http
GET https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v1/management/deviceclasses/{deviceClassId}

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
|  200 OK | The details of the device class |
|  404 NOT FOUND | Device class not found |

Name|Type|Description
----|------|------|
DeviceClassId| String|Unique identifier for the device.
Manufacturer| String|Manufacturer of the device class
Model| String | Model name of the device class
bestCompatibleUpdateId | UpdateId | The best compatible updateId for the device class

```json
{
  "deviceClassId": "d74e3c87fbf98074c20c3269f1c9e58d255906dd",
  "manufacturer": "Contoso",
  "model": "Virtual-Machine",
  "bestCompatibleUpdateId": {
    "provider": "Contoso",
    "name": "Virtual-Machine",
    "version": "10.0"
  }
}
```
