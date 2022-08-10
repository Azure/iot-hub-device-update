# Device Update Agent Provisioning

The Device Update Module agent can run alongside other system processes and [IoT Edge modules](../iot-edge/iot-edge-modules.md) that connect to your IoT Hub as part of the same logical device. This section describes how to provision the Device Update agent as a module identity. 

## Module identity vs device identity

In IoT Hub, under each device identity, you can create up to 50 module identities. Each module identity implicitly generates a module twin. On the device side, the IoT Hub device SDKs enable you to create modules where each one opens an independent connection to IoT Hub. Module identity and module twin provide the similar capabilities as device identity and device twin but at a finer granularity. [Learn more about Module Identities in IoT Hub](../iot-hub/iot-hub-devguide-module-twins.md)

If you are migrating from a device level agent to adding the agent as a Module identity on the device, remove the older agent that was communicating over the Device Twin. When you provision the Device Update agent as a Module Identity, all communications between the device and the Device Update service happen over the Module Twin so do remember to tag the Module Twin of the device when creating [groups](device-update-groups.md) and all [communications](device-update-plug-and-play.md) must happen over the module twin.

## Support for Device Update

The following IoT device over the air update types are currently supported with Device Update:

* Linux devices (IoT Edge and Non-IoT Edge devices):
    * [Image A/B update](device-update-raspberry-pi.md)       
    * [Package update](device-update-ubuntu-agent.md)
    * [Proxy update for downstream devices](device-update-howto-proxy-updates.md)
	
* Constrained devices:
    * AzureRTOS Device Update agent samples: [Device Update for Azure IoT Hub tutorial for Azure-Real-Time-Operating-System](device-update-azure-real-time-operating-system.md)

* Disconnected devices: 
    * [Understand support for disconnected device update](connected-cache-disconnected-device-update.md)


## Prerequisites  

If you're setting up the IoT device/IoT Edge device for [package based updates](./understand-device-update.md#support-for-a-wide-range-of-update-artifacts), add packages.microsoft.com to your machine’s repositories by following these steps:

1. Log onto the machine or IoT device on which you intend to install the Device Update agent.

1. Open a Terminal window.

1. Install the repository configuration that matches your device’s operating system.

    ```shell
    curl https://packages.microsoft.com/config/ubuntu/18.04/multiarch/prod.list > ./microsoft-prod.list
    ```
    
1. Copy the generated list to the sources.list.d directory.

    ```shell
    sudo cp ./microsoft-prod.list /etc/apt/sources.list.d/
    ```
    
1. Install the Microsoft GPG public key.

    ```shell
    curl https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor > microsoft.gpg
    ```

    ```shell
    sudo cp ./microsoft.gpg /etc/apt/trusted.gpg.d/
    ```

## How to provision the Device Update agent as a Module Identity

This section describes how to provision the Device Update agent as a module identity on 
* IoT Edge enabled devices, or 
* Non-Edge IoT devices, or
* Other IoT devices. 

To check if you have IoT Edge enabled on your device, please refer to the [IoT Edge installation instructions](../iot-edge/how-to-provision-single-device-linux-symmetric.md?preserve-view=true&view=iotedge-2020-11).
 
Follow all or any of the below sections to add the Device update agent based on the type of IoT device you are managing. 

### On IoT Edge enabled devices

Follow these instructions to provision the Device Update agent on [IoT Edge enabled devices](../iot-edge/index.yml).

1. Follow the instructions to [Manually provision a single Linux IoT Edge device](../iot-edge/how-to-provision-single-device-linux-symmetric.md?preserve-view=true&view=iotedge-2020-11#install-iot-edge).

1. Install the Device Update image update agent.

    We provide sample images in the [Assets here](https://github.com/Azure/iot-hub-device-update/releases) repository. The swUpdate file is the base image that you can flash onto a Raspberry Pi B3+ board. The .gz file is the update you would import through Device Update for IoT Hub. For an example, see [How to flash the image to your IoT Hub device](./device-update-raspberry-pi.md#flash-an-sd-card-with-the-image).  

1. Install the Device Update package update agent.

    - For any early access agent version from [Artifacts](https://github.com/Azure/iot-hub-device-update/releases) : Download the deviceupdate-agent_0.9.0 and deliveryoptimization-agent deb files to the machine you want to install the Device Update and Delivery Optimization agent on, then:
   
        ```shell
        sudo apt-get install -y ./"<PATH TO FILE>"/"<.DEB FILE NAME>"
        ```
	
1. You are now ready to start the Device Update agent on your IoT Edge device. 

### On Iot Linux devices without IoT Edge installed

Follow these instructions to provision the Device Update agent on your IoT Linux devices.

1. Install the IoT Identity Service and add the latest version to your IoT device by following instrucions in [Installing the Azure IoT Identity Service](https://azure.github.io/iot-identity-service/installation.html#install-from-packagesmicrosoftcom).

2. Configure the IoT Identity Service by following the instructions in [Configuring the Azure IoT Identity Service](https://azure.github.io/iot-identity-service/configuration.html). 
    
3. Finally install the Device Update agent. We provide sample images in [Assets here](https://github.com/Azure/iot-hub-device-update/releases), the swUpdate file is the base image that you can flash onto a Raspberry Pi B3+ board, and the .gz file is the update you would import through Device Update for IoT Hub. See example of [how to flash the image to your IoT Hub device](./device-update-raspberry-pi.md#flash-an-sd-card-with-the-image).

4. After you've installed the device update agent, you will need to edit the configuration file for Device Update by running the command below. 

    ```shell
   	sudo nano /etc/adu/du-config.json
    ```
   Change the connectionType to "AIS" for agents who will be using the IoT Identity Service for provisioning. The ConnectionData field must be a empty string

5. You are now ready to start the Device Update agent on your IoT device. 

### Other IoT devices

The Device Update agent can also be configured without the IoT Identity service for testing or on constrained devices. Follow the below steps to provision the Device Update agent using a connection string (from the Module or Device).

1. Log onto the machine or IoT Edge device/IoT device.
	
1. Open a terminal window.

1. Add the connection string to the [Device Update configuration file](device-update-configuration-file.md):

    1. Enter the below in the terminal window:
   
        - [For Ubuntu agent](device-update-ubuntu-agent.md) use: sudo nano /etc/adu/du-config.json
       
    1. Copy the primary connection string
    
    	- If Device Update agent is configured as a module copy the module's primary connection string. 
    	- Otherwise copy the device's primary connection string.
     
    3. Enter the copied primary connection string to the 'connectionData' field's value in the du-config.json file. Then save and close the file.
 
1. Now you are now ready to start the Device Update agent on your IoT device. 

## How to start the Device Update Agent

This section describes how to start and verify the Device Update agent as a module identity running successfully on your IoT device.

1. Log into the machine or device that has the Device Update agent installed.

1. Open a Terminal window, and enter the command below.

    ```shell
    sudo systemctl restart adu-agent
    ```
    
1. You can check the status of the agent using the command below. If you see any issues, refer to this [troubleshooting guide](troubleshoot-device-update.md).
	
    ```shell
    sudo systemctl status adu-agent
    ```
    
    You should see status OK.

1. On the IoT Hub portal,  go to IoT device or IoT Edge devices to find the device that you configured with Device Update agent. There you will see the Device Update agent running as a module. 
   

## How to build and run Device Update Agent

You can also build and modify your own customer Device Update agent.

Follow the instructions to [build](https://github.com/Azure/iot-hub-device-update/blob/main/docs/agent-reference/how-to-build-agent-code.md) the Device Update Agent
from source.

Once the agent is successfully building, it's time to [run](https://github.com/Azure/iot-hub-device-update/blob/main/docs/agent-reference/how-to-run-agent.md)
the agent.

Now, make the changes needed to incorporate the agent into your image.  Look at how to
[modify](https://github.com/Azure/iot-hub-device-update/blob/main/docs/agent-reference/how-to-modify-the-agent-code.md) the Device Update Agent for guidance.


## Troubleshooting guide

If you run into issues, review the Device Update for IoT Hub [Troubleshooting Guide](troubleshoot-device-update.md) to help unblock any possible issues and collect necessary information to provide to Microsoft.

