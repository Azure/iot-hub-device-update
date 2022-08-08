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

## Azure IoT/ADU CLI instructions

1. Ensure you have the ADU bash CLI https://github.com/Azure/azure-iot-cli-extension/wiki/ADU-CLI-Primer  
Device twin support is available via normal azure-iot extension which can be installed via "az extension add --source azure-iot" but the ADU bash CLI is recommended to enable ADU data-plane functionality. We recommend Azure CLI 2.30.0 or greater.
2. You can update a single device twin tag via https://docs.microsoft.com/en-us/cli/azure/iot/hub/device-twin?view=azure-cli-latest#az-iot-hub-device-twin-update
	    ```
        Example (powershell): 
			§ az iot hub device-twin update -n hubName -d myDevice --tags '{\"ADUGroup\": \"ADUBBTag1\"}'
        ```
        ```
		Example (bash):  
			§ az iot hub device-twin update -n hubName -d myDevice --tags '{"ADUGroup": "ADUBBTag1"}'
         ```
         ```
		 Example (shell agnostic): 
			§ az iot hub device-twin update -n hubName -d myDevice --tags '/path/to/tag/payload/file'
         ```
3. You can update a set of device twins (using a twin query) via https://docs.microsoft.com/en-us/cli/azure/iot/hub/job?view=azure-cli-latest#az-iot-hub-job-create
		```
        Example (powershell): 
			§ az iot hub job create --job-id test1 --job-type scheduleUpdateTwin -n hubName -q "*" --twin-patch '{\"tags\":{\"ADUGroup\": \"ADUBBTag1\"}}' --wait
		```
        ```
        Example (bash):
			§ az iot hub job create --job-id test1 --job-type scheduleUpdateTwin -n hubName -q "*" --twin-patch '{"tags":{"ADUGroup": "ADUBBTag1"}}' --wait
        ```
4. Now you can execute the "az iot device-update device import" ADU CLI command to import IoT Hub devices to the target ADU instance https://github.com/Azure/azure-iot-cli-extension/wiki/ADU-CLI-Primer#importing-and-managing-devices
		```
        az iot device-update device import --account test-adu-35d878a5c16248 --instance myinstance1
		```



