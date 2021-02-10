# Get Update

Gets details of an update version that has been imported into the ADU instance. Content in ADU has a 3-part identity: provider, name, and version.

## HTTP Request

```http
GET https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v2/updates/providers/{provider}/names/{name}/versions/{version}
```

## URI Parameters

| Name | In | Required | Type | Description |
| --------- | --------- | --------- | --------- | --------- |
| {account} | hostname | True | string | ADU account name |
| {instance}| path | True | string | ADU instance name |
| {provider} | path | True | string | Provider part of the update identity |
| {name} | path | True | string | Name part of the update identity |
| {version} | path | True | version | Version part of the update identity |

## Request Headers

| Name | Required | Description |
| --------- | --------- | --------- |
| Authorization | True | OAuth2 header containing the caller’s AAD token. |

**Permissions** </br>
To call this API, the caller must supply a valid AAD token from the Azure tenant associated with the requested ADU account and instance. The AAD account must be a member of the ADU FullAccessAdmin role.

## Responses

| HTTP Code | Description |
| --------- | --------- |
| 200 OK | `Update` object |
| 304 | Not modified |
| 404 | Not found |
| 429 | Too many requests; there is a rate limit on how many operations can be executed within a time period |

```json
{
    "updateId": {
        "provider": "Contoso",
        "name": "Virtual-Machine",
        "version": "1.0"
    },
    "updateType": "microsoft/swupdate:1",
    "installedCriteria": "10",
    "compatibility": [
        {
            "deviceManufacturer": "Contoso",
            "deviceModel": "Toaster"
        }
    ],
    "manifestVersion": "2.0",
    "createdDateTime": "2020-05-07T23:01:14.984Z",
    "importedDateTime": "2020-05-07T23:37:58.7115094Z",
    "etag": "\"0f7ea3de-a342-4103-b874-c80173f6bdf9\""
}
```

## Definitions

### Update Object

| Name | Type | Description |
| --------- | --------- | --------- |
| UpdateId | `UpdateId` object | Update identity. |
| UpdateType | string | Update type. |
| InstalledCriteria | string | String interpreted by the agent to determine if the update completed successfully. |
| Compatibility | array of `CompatibilityInfo` objects | Compatibility information of device this update is compatible with. |
| ManifestVersion | string | Import manifest schema version. |
| CreatedDateTime | date/time | Date/time in UTC at which the update was created. |
| ImportedDateTime | date/time | Date/time in UTC at which the update was imported. |
| ETag | string | HTTP Entity tag. |

### UpdateId Object

| Name | Type | Description |
| --------- | -----------|-------------- |
| Provider | string | Provider part of the update identity. |
| Name | string | Name part of the update identity. |
| Version | version | Version part of the update identity. |

### CompatibilityInfo Object

| Name | Type | Description |
| --------- | -----------|-------------- |
| DeviceManufacturer | string | Manufacturer of the device the update is compatible with. |
| DeviceModel | string | Model of the device the update is compatible with. |