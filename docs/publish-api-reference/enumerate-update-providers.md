# Enumerate Update Providers

Gets a list of all unique providers of update that have been imported to the ADU instance. Update in ADU has a 3-part identity: provider, name, and version.

## HTTP Request

```http
GET https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v2/updates/providers
```

## URI Parameters

| Name | In | Required | Type | Description |
| --------- | --------- | --------- | --------- | --------- |
| {account} | hostname | True | string | ADU account name |
| {instance}| path | True | string | ADU instance name |

## Request Headers

| Name | Required | Description |
| --------- | --------- | --------- |
| Authorization | True | OAuth2 header containing the caller’s AAD token. |

Permissions </br>
To call this API, the caller must supply a valid AAD token from the Azure tenant associated with the requested ADU account and instance. The AAD account must be a member of the ADU FullAccessAdmin role.

## Responses

| HTTP Code | Description |
| --------- | --------- |
| 200 OK | All available update providers |
| 429 TOO MANY REQUESTS | Too many requests; there is a rate limit on how many operations can be executed within a time period |

```json
{
  "value": [
    "Contoso",
    "Fabrikam",
  ]
}
```