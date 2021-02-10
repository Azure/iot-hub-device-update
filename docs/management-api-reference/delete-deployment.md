# Delete Deployment

Deletes the deployment from the ADU instance.

## HTTP Request

```http
DELETE https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v2/management/deployments/{deploymentId}
```

## URI Parameters

Name|Input|Required|Type|Description
---|---|---|---|---
{account} | hostname | True | string | ADU account name
{instance} | hostname | True | string | ADU instance name
{deploymentId} | path | True | string | The deploymentId returned by the Get deployment API

## Request Headers

Name|Required|Description
---|---|---
Authorization | True | Authorization request header with the caller's AAD token passed as an OAuth 2.0 bearer token. |

## Request Body

The request body should be empty.

## Responses

Name|Type|Description
---|---|---
200 OK | deployment | Deployment was deleted successfully (synchronously).
404 NOT FOUND | none | Deployment not found.
