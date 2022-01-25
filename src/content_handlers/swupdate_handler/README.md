# SWUpdate Update Handler

SWUpdate handler is a reference implementation of an Update Content Handler that enables a device to support an Image-based Update (A/B)

> Note | This handler is provided for demonstration purposes only.

This handler invokes tasks provided by the [adu-shell](../../adu-shell) command-line tool, which acts as a broker for executing specific tasks as `root`.  

For some tasks, `adu-shell` will execute [adu-swupdate.sh](../../adu-shell/scripts) script, which implements the required business logic for:

- Update image signature validation
- Update image installation
- Boot loader configuration
- Device reboot  

> Note | This implementation may not be applicable with your device configuration (e.g., mismatch partitions layout). To support your device, some customization may be required.

For more information, see [Device Update for Azure IoT Hub tutorial using the Raspberry Pi 3 B+ Reference Image](https://docs.microsoft.com/en-us/azure/iot-hub-device-update/device-update-raspberry-pi)
