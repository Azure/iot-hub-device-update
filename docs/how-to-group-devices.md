# How to Group devices

This step is optional when trying to deploy updates to your managed devices starting with this release. You can deploy updates to your devices using the default group that is created for you. Alternatively, you can assign a user-defined tag to your devices, and they will be automatically grouped based on the tag and the device compatibility properties. 

## Tag devices

There are multiple ways to do this. Refer to ["Add a tag to your devices" section](https://docs.microsoft.com/en-us/azure/iot-hub-device-update/create-update-group)
Follow the steps below to manually tag your devices.

1. Go to the [Azure Portal](https://ms.portal.azure.com/?feature.adu=true&feature.canmodifystamps=true&Microsoft_Azure_Iothub=tip1&Microsoft_Azure_ADU_Diagnostic=true) and select the IoT Hub icon
2. Select the IoT Hub you have connected to your ADU instance where you would like to test the early access release
3. From 'IoT Devices' on the left navigation pane find your IoT device, select the Device Update Module and then its Module Twin 
4. Add a new Device Update tag value as shown below
	```
    "tags": {
	            "ADUGroup": "<CustomTagValue>"
	            }
    ```
