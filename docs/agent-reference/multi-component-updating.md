# Proxy Updates and multi-component updating

This document explains how Proxy Updates, using the multi-step ordered execution (MSOE) feature, can update multiple components on the IoT device or sensors connected to it. The Device Update agent on the main device facilitates the update process for the connected sensors since they cannot connect to the IoT hub directly. Proxy Updates are useful in scenarios where multiple components or sensors require updates. Examples of such scenarios include:

* Targeting specific update files to different partitions on the device.
* Targeting specific update files to different apps/components on the device  
* Targeting specific update files to sensors connected to IoT devices over a network protocol (e.g., USB, CANbus etc.).

To update components on an IoT device that is connected to an IoTHub, also known as the **Host Device** in this document, the device builder needs to register a custom **Component Enumerator Extension** designed for their device. This registration is necessary for the Device Update Agent to associate a 'child update' with a specific component or group of components targeted for the update.

It's crucial to note that the Device Update service does not have any knowledge of the specific components on the target device since it's designed to be device agnostic. Therefore, the device builder must provide the necessary information to the Device Update Agent. This information includes a collection of components such as root file system partitions, firmware partitions, boot partition, or any applications or group of applications that are modeled virtually as a 'component'. The device builder can provide this information to the Agent by developing a Component Enumerator extension and registering it with the Agent.

> Note: Multi-step ordered execution feature allow for granular update controls including an install order, pre-install, install and post-install steps. Use cases include
> a required preinstall check that is needed to validate the device state before starting an update, etc. Learn more about [multi-step ordered execution](../agent-reference/update-manifest-v4-schema.md).

## Implementing the Component Enumerator Extension

To provide information about components on the target device to the Device Update Agent, you can refer to the [Contoso Component Enumerator example](../../src/extensions/component_enumerators/examples/contoso_component_enumerator/README.md). This example provides guidance on how to implement and register a custom Component Enumerator extension.

## Proxy Update Example

See [tutorial using the Device Update agent to do Proxy Updates](../../src/extensions/component_enumerators/examples/contoso_component_enumerator/demo/README.md) with sample updates for components connected to a Contoso Virtual Vacuum device.
