# Cancel or Retry Deployment

Cancel or retry failed devices for a deployment.

## HTTP Request

```http
POST https://{account}.api.adu.microsoft.com/deviceupdate/{instance}/v2/management/deployments/{deploymentId}?action=cancel
```

## URI Parameters

Name|Input|Required|Type|Description
---|---|---|---|---
{account} | hostname | True | string | ADU account name
{instance} | hostname | True | string | ADU instance name
{deploymentId} | path | True | string | The deploymentId returned by the Get deployment API
action | path | True | string | The action to take on the deployment. To cancel, set the value to `cancel`.  To retry failed devices, set the value to `retry`

## Request Headers

Name|Required|Description
---|---|---
Authorization | True | Authorization request header with the caller's AAD token passed as an OAuth 2.0 bearer token. |

## Request Body

The request body should be empty.

## Responses

Name|Type|Description
---|---|---
200 OK | deployment | Deployment was successfully canceled (synchronously). The deployment's status is returned as a `Deployment` object.
404 NOT FOUND | none | Deployment not found.

## Definitions

### Deployment object

Name|Required|Type|Description
---|---|---|---
DeploymentId | True | string | The Deployment ID.
DeviceClassId | True | string | The device class ID.
DeviceGroupType | True | DeviceGroupType enum | Indicates the group type of the deployment. Valid values are `All`, `Devices`, and `DeviceGroupDefinitions`.
DeploymentType | True | DeploymentType enum | Indicates the type of the deployment. The only valid values is `Complete`.
DeviceGroupDefinition | True | Array of strings | List of group definitions associated with the deployment.
StartDateTime | True | DateTime | Beginning of the time window during which the deployment may start.
EndDateTime | True | DateTime | End of the time window during which the deployment may start.
ContentId | True | string | The ContentId of the deployment.
IsCanceled | True | Boolean | Flag indicating whether the deployment has been requested to be canceled.
