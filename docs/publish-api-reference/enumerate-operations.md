# Enumerate Operations

Gets a list of all import and delete operations. Completed operations are kept for 7 days.

## HTTP Request

```http
GET https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v2/updates/operations
```

## URI Parameters

| Name | In | Required | Type | Description |
| --------- | --------- | --------- | --------- | --------- |
| {account} | hostname | True | string | ADU account name |
| {instance}| path | True | string | ADU instance name |
| $top | path | False | int | OData `$top` query option |
| $filter | path | False | int | OData `$filter` query option. Only one specific filter is supported: `status eq 'NotStarted' or status eq 'Running'` |

## Request Headers

| Name | Required | Description |
| :--------- | :-----------|:-------------- |
| Authorization | True | OAuth2 header containing the caller’s AAD token. |

Permissions </br>
To call this API, the caller must supply a valid AAD token from the Azure tenant associated with the requested ADU account and instance. The AAD account must be a member of the ADU FullAccessAdmin role.

## Responses

| HTTP Code | Description |
| :--------- | :---- |
| 200 OK | Paginated list of [`Operation`](get-operation.md#operation-object) object, with link to next page specified in `nextLink` field (if any) |
| 429 TOO MANY REQUESTS | Too many requests; there is a rate limit on how many operations can be executed within a time period |

```json
{
    "value": [
        {
            "operationId": "b8db5010-3d40-4765-b8dd-1f709efd79dd",
            "status": "Succeeded",
            "updateId": {
                "provider": "Contoso",
                "name": "ImageUpdate",
                "version": "3.7"
            },
            "resourceLocation": "/deviceupdate/blue/v2/updates/providers/Contoso/names/ImageUpdate/versions/3.7",
            "traceId": "01f968658d5f47b6b08f012b92139d42",
            "lastActionDateTime": "2020-10-06T16:54:51.0096896Z",
            "createdDateTime": "2020-10-06T16:50:48.3463238Z",
            "etag": "\"bb403e37-6b82-438b-b48b-684e21a94b1d\""
        }
    ],
    "nextLink": "/deviceupdate/blue/v2/updates/operations?$top=100&$skipToken=%5B%7B%22token%22%3A%22%2BRID%3A%7EfFJ8AI04QtA0CwAAAAAAAA%3D%3D%23RT%3A1%23TRC%3A20%23RTD%3AvEB3Goh2hqn%2BvS7NiHWvBTMxMzEuMjEuMTdVMTM7Mzc7NTIvNjczMTcxNFsA%23ISV%3A2%23IEO%3A65551%23FPC%3AAgEAAABUADuJEMCIAAAkAAClAAAEAEACABAAACkAACkAAAEAEAAACQBAqhKAEQCAIhCAE8CQAgAAFAoAQAAAJAAACQBAAgCQAAAkIAIAkAAApAKiCgBAAgABAA%3D%3D%22%2C%22range%22%3A%7B%22min%22%3A%22%22%2C%22max%22%3A%22FF%22%7D%7D%5D"
}
```