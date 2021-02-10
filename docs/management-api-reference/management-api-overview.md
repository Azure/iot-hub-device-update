# Management APIs

The management APIs enable you to select applicable updates for devices, deploy updates to devices and then view the status of updates during the deployment process.  You are also able to add or remove devices from device groups as well as being able to view the update compliance of devices and groups.

## Getting Started

### Selecting applicable updates for devices

After update has been published to ADU, the first item to do before creating a deployment is to match a device class to installable updates for a device. 
* To do this first call [Get Device](get-device.md) to get the details including the device class id for the device you want to update.
* Once you have found the specific device class you are interested in, then use the [Get Installable Update for a Device Class](get-installable-update-for-device-class.md) to view the updates for a given Device Class.

### Creating a deployment

Once you have identified the applicable updates for a device, you will then create a deployment. To create a deployment use the [Create or Update Deployment](create-or-update-deployment.md). This api will let you specify the update, device class, and also whether to deploy to a set of specific devices or use a group Id to set the devices that will receive an update.

### Viewing the state of a deployment

Once an update has been deployed, you can view the detailed status of a deployment, to get general status (active, canceled), using [Get Deployment Status](get-deployment-status.md). To view the details of a specific deployment that was created use [Get Deployment Details](get-deployment-details.md).

### Verify update has been deployed to a device

After an update has completed or any time you can view the installed update on a device by calling [Get Device](get-device.md)

## Complete List Of Management APIs

* [Get Device Classes](get-device-classes.md)
* [Get Device Class Details](get-device-class-details.md)
* [Get Device Ids in Device Class](get-device-ids-in-device-class.md)
* [Get Installable Update for a Device Class](get-installable-update-for-device-class.md)
* [Get Devices](get-devices.md)
* [Get Device](get-device.md)
* [Get Available Device Tags](get-available-device-tags.md)
* [Get Device Tag](get-device-tag.md)
* [Get Groups](get-groups.md)
* [Get Group](get-group.md)
* [Get Group Update Compliance](get-group-update-compliance.md)
* [Get Group Best Updates](get-group-best-updates.md)
* [Get Update Compliance](get-update-compliance.md)
* [Create or Update Deployment](create-or-update-deployment.md)
* [Cancel or Retry Deployment](cancel-or-retry-deployment.md)
* [Delete Deployment](delete-deployment.md)
* [Get Deployments](get-deployments.md)
* [Get Deployment Status](get-deployment-status.md)
* [Get Deployment Details](get-deployment-details.md)
* [Get Deployment Device States](get-deployment-device-states.md)
