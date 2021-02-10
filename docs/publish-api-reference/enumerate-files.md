# Enumerate Files

Gets a list of file ids for an update version that has been imported into the ADU instance. Updates in ADU have a 3-part identity: provider, name, and version.

## HTTP Request

```http
GET https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v2/updates/providers/{provider}/names/{name}/versions/{version}/files
```

## URI Parameters

| Name | In | Required | Type | Description |
| --------- | --------- | --------- | --------- | --------- |
| {account} | hostname | True | string | ADU account name |
| {instance}| path | True | string | ADU instance name |
| {namespace} | path | True | string | Provider part of the update identity |
| {name} | path | True | string | Name part of the update identity |
| {version} | path | True | version | Version part of the update identity |

## Request Headers

| Name | Required | Description |
| --------- | --------- | --------- | --------- | --------- |
| Authorization | True | OAuth2 header containing the caller’s AAD token. |

Permissions </br>
To call this API, the caller must supply a valid AAD token from the Azure tenant associated with the requested ADU account and instance. The AAD account must be a member of the ADU FullAccessAdmin role.

## Responses

| HTTP Code | Description |
| --------- | --------- |
| 200 OK | The array of all file ids that belong to the given update version |
| 404 NOT FOUND | Not found |
| 429 TOO MANY REQUESTS | Too many requests; there is a rate limit on how many operations can be executed within a time period |

```json
{
  "value": [
    "00000",
    "00001",
  ]
}
```
