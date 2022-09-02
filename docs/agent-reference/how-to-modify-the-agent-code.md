# How To Modify the Device Update Agent Code

Here are some cases where you might want to modify the Device Update Agent code to fit your scenarios and devices.

## Adding additional IoT Plug and Play interfaces

You may want to implement your own Azure IoT PnP interfaces in addition to the
ones necessary for Device Update for IoT Hub.  We recommend looking at how the ADU Core Interface and Device Information interfaces are implemented in
[src/agent/adu_core_interface](../../src/agent/adu_core_interface). Specifically
[adu_core_interface.h](../../src/agent/adu_core_interface/inc/aduc/adu_core_interface.h)
and
[adu_core_interface.c](../../src/agent/adu_core_interface/src/adu_core_interface.c).

## Adjusting the Download, Install, and Apply actions for your device

You may want to implement your own version of IsInstalled, Download, Install and Apply for
your device or installer. You can provide your own implementation of the
[Content Handler APIs](../../src/extensions/inc/aduc/content_handler.hpp)
in [src/extensions/step_handlers](../../src/extensions/step_handlers)

## Customize extensibility points

In addition to customizing content handlers, you can customize:
- How update steps in the update metadata are orchestrated/processed,
- How update payloads and detached v4 update manifest files are downloaded,
- How hardware sub-module "components" are enumerated and queried,
- How update payloads can be pre-processed to reconstitute payloads (particularly large ones) without downloading them (e.g. Delta Updating).

For comprehensive docs on all extensibility points, refer to the [Device Update Agent Extensibility points](./device-update-agent-extensibility-points.md) doc.

## Porting to a Different OS or Platform

If you want to use the Device Update Agent on an OS or platform that isn't supported out-of-the box, you can provide your own [Platform Layer](../../src/platform_layers) implementation. You may also need to port the [Azure IoT C SDK](https://github.com/Azure/azure-c-shared-utility/blob/master/devdoc/porting_guide.md) to your device.

1. Follow these steps to get the Device Update sample application running on other boards

* Implement the Azure IoT device SDK for C, see an example for Arduino [here](https://github.com/Azure/azure-iot-sdk-c/blob/master/iothub_client/readme.md#arduino).
* Implement the [IoT Hub Plug and Play Device Client Sample](https://github.com/Azure/azure-iot-sdk-c/tree/master/iothub_client/samples/pnp).
* Once you have a IoT Hub Plug and Play enabled device, implement the 'ADU core' interfaces for your application by following [this source code](../../src/adu_workflow/src/agent_workflow.c).

2. Once you get a basic IoT Hub Plug and Play Device Client sample up and running on the device, look at our current Device Update Agent implementation to implement over-the-air ‘Update Actions’ using the Device Update Service.

* [Understand Device Update for Azure IoT Hub Agent](https://docs.microsoft.com/azure/iot-hub-device-update/device-update-agent-overview)
* [Understand how Device Update for IoT Hub uses IoT Plug and Play](https://docs.microsoft.com/azure/iot-hub-device-update/device-update-plug-and-play)
* [IoT Hub Device Update Public Preview source code](../../src)
