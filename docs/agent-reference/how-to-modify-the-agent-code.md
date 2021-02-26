# How To Modify the Device Update for IoT Hub Agent Code

Here are some cases where you might want to modify the Device Update Agent code to fit your scenarios and devices.

## Adding additional IoT Plug and Play interfaces

You may want to implement your own Azure IoT PnP interfaces in addition to the
ones necessary for Device Update for IoT Hub.  We recommend looking at how the ADU Core Interface and Device Information interfaces are implemented in
[src/agent/adu_core_interface](../../src/agent/adu_core_interface). Specifically
[adu_core_interface.h](../../src/agent/adu_core_interface/inc/aduc/adu_core_interface.h)
and
[adu_core_interface.c](../../src/agent/adu_core_interface/src/adu_core_interface.c).

## Adjusting the Download, Install, and Apply actions for your device

You may want to implement your own version of Download, Install and Apply for
your device or installer. You can provide your own implementation of the
[Content Handler APIs](../../src/content_handlers/inc/aduc/content_handler.hpp)
in [src/content_handlers](../../src/content_handlers)

## Porting to a Different OS or Platform

If you want to use the Device Update Agent on an OS or platform that isn't supported out-of-the box, you can provide your own [Platform Layer](../../src/platform_layers) implementation. You may also need to port the [Azure IoT C SDK](https://github.com/Azure/azure-c-shared-utility/blob/public-preview/devdoc/porting_guide.md) to your device.
