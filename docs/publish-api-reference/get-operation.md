# Get Operation

Gets status of update import or delete operation.

## HTTP Request

```http
GET https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v2/updates/operations/{operationId}
```

## URI Parameters

| Name | In | Required | Type | Description |
| --------- | --------- | --------- | --------- | --------- |
| {account} | hostname | True | string | ADU account name |
| {instance}| path | True | string | ADU instance name |
| {operationId} | path | True | string | Operation Id returned in `Operation-Location` response header of `Import Update` API |

## Request Headers

| Name | Required | Description |
| --------- | --------- | --------- |
| Authorization | True | OAuth2 header containing the caller’s AAD token. |

**Permissions** </br>
To call this API, the caller must supply a valid AAD token from the Azure tenant associated with the requested ADU account and instance. The AAD account must be a member of the ADU FullAccessAdmin role.

## Responses

| HTTP Code | Description |
| --------- | --------- |
| 200 OK | `Operation` object |
| 304 | Not modified |
| 404 | Not found |
| 429 | Too many requests; there is a rate limit on how many operations can be executed within a time period |

### Successful Operation
```json
{
    "operationId": "bf50f3f9-892b-45d3-9108-14c1bf8c6b03",
    "status": "Succeeded",
    "updateId": {
        "provider": "Contoso",
        "name": "ImageUpdate",
        "version": "3.7"
    },
    "resourceLocation": "/deviceupdate/blue/v2/updates/providers/Contoso/names/ImageUpdate/versions/3.7",
    "traceId": "fd4f33287fc2454086125e48318d93a5",
    "lastActionDateTime": "2020-10-08T22:42:50.2031375Z",
    "createdDateTime": "2020-10-08T22:40:46.1568243Z",
    "etag": "\"3ce7663a-3c0f-4d12-908d-8b02dd16bfb5\""
}
```

### Failing Operation
```json
{
    "operationId": "adf1a2d5-24d7-48ef-b20e-acc859d608fc",
    "status": "Failed",
    "error": {
        "occurredDateTime": "2020-10-06T21:44:33.8304014Z",
        "code": "CannotProcessImportManifest",
        "message": "Error processing import manifest.",
        "innerError": {
            "code": "CannotDownload",
            "message": "Cannot download import manifest.",
            "errorDetail": "Response status code does not indicate success: 403 (Server failed to authenticate the request. Make sure the value of Authorization header is formed correctly including the signature.)."
        }
    },
    "traceId": "806bc43533dfcb4491059fbd930fe454",
    "lastActionDateTime": "2020-10-06T21:44:33.8971055Z",
    "createdDateTime": "2020-10-06T21:44:33.1225747Z",
    "etag": "\"de2ee670-d657-4ddf-9e30-b48e1cc43a3c\""
}
```

## Definitions

### Operation Object

| Name | Type | Description |
| --------- | --------- | --------- |
| OperationId | string | Operation identity |
| Status | string | Operation status. Possible values: `NotStarted`, `Running`, `Succeeded`, `Failed` |
| UpdateId | `UpdateId` object | Identity of update being imported or deleted. For import, this property will only be populated after import manifest is processed successfully. |
| ResourceLocation | URL | Location of the new update on successful import operation |
| Error | `Error` object | Error detail of failing operation |
| TraceId | string | Operation correlation identity that can used by Microsoft Support for troubleshooting |
| LastActionDateTime | date/time | Date/time in UTC at which the operation was last updated. |
| CreatedDateTime | date/time | Date/time in UTC at which the operation was created. |
| ETag | string | HTTP Entity tag. |

### Error Object

| Name | Type | Description |
| --------- | -----------|-------------- |
| OccuredDateTime | date/time | Date/time in UTC at which the error occured |
| Code | string | Error code |
| Message | string | Description of the error code |
| InnerError | `InnerError` object | Detailed error information, if any |

### InnerError Object

| Name | Type | Description |
| --------- | -----------|-------------- |
| Code | string | Error code |
| Message | string | Description of the error code |
| ErrorDetail | string | Detailed error message |
| InnerError | `InnerError` object | Detailed error information, if any |