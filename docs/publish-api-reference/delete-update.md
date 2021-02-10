# Delete Update

Deletes an update version from the ADU instance.

## HTTP Request

```http
DELETE https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v2/updates/providers/{provider}/names/{name}/versions/{version}
```

## URI Parameters

| Name | In | Required | Type | Description |
| --------- | --------- | --------- | --------- | --------- |
| {account} | hostname | True | string | ADU account name |
| {instance} | path | True | string | ADU instance name |
| {provider} | path | True | string | Provider part of the update identity |
| {name} | path | True | string | Name part of the update identity |
| {version} | path | True | version | Version part of the update identity |

## Request Headers

| Name | Required | Description |
| --------- | --------- | --------- |
| Authorization | True | OAuth2 header containing the caller’s AAD token. |

Permissions </br>
To call this API, the caller must supply a valid AAD token from the Azure tenant associated with the requested ADU account and instance. The AAD account must be a member of the ADU FullAccessAdmin role.

## Responses

| HTTP Code | Description |
| --------- | --------- |
| 202 ACCEPTED | Accepted update delete request and background operation URL is specified in `Operation-Location` response header |
| 429 TOO MANY REQUESTS | Too many requests; there is a rate limit on how many operations can be executed within a time period and there is also a limit on how many concurrent import and delete background operations can be executed. |

