---
title: Device Update for Azure IoT Hub agent check | Microsoft Docs
description: Device Update for IoT Hub uses Agent Check to find and diagnose missing devices.
author: lichris
ms.author: lichris
ms.date: 8/8/2022
ms.topic: how-to
ms.service: iot-hub-device-update
---

# Find and fix devices missing from Device Update for IoT Hub using Agent Check

Learn how to use Agent Check to find, diagnose, and fix devices missing from your Device Update for IoT Hub instance.

## Prerequisites

* [Access to an IoT Hub with Device Update for IoT Hub enabled](create-device-update-account.md).
* An IoT device (or simulator) [provisioned for Device Update](device-update-agent-provisioning.md) and reporting a compatible PnP Model Id.

> [!NOTE]
> The Agent Check feature can only perform validation checks on devices that have the Device Update agent installed and are reporting a PnP Model Id that matches those compatible with Device Update for IoT Hub.

---

## Validation checks supported by Agent Check

The Agent Check feature currently performs the following validation checks on all devices that meet the above pre-requisites.

| Validation check                      | Criteria                                                                                                           |
|---------------------------------------|--------------------------------------------------------------------------------------------------------------------|
| PnP Model Id                          | Must be a valid digital twin model identifier (DTMI) that supports the Device Update interface.                    |
| Interface Id                          | "deviceUpdate.agent.deviceProperties[interfaceId]" must be a valid DTMI that supports the Device Update interface. |
| Compatibility Property Names          | Must be a comma-delimited list of strings. The list must contain at least one and no more than 5 strings. Each string must be < 32 characters. |
| Compatibility Property Values         | "deviceUpdate.agent.deviceProperties" is a dictionary/map. The dictionary must contain, as a key, every property defined in "Compatibility Property Names". The value of each dictionary item is limited to 64 characters. |
| ADU Group                             | The ADUGroup tag is optional. If specified, the tag string is limited to 255 characters and may only contain alphanumeric characters and the following special characters: "." "-" "_" "~" |

If a device fails any of these criteria, it may not show up properly in Device Update. Correcting the invalid value to meet the specified critiera should cause the device to properly appear in Device Update. If the device does not show up in Device Update **nor** in Agent Check, you may need to run Device Sync to resolve the issue.

## View Agent Check results

The results of Agent Check can be found by navigating to the Diagnostics tab of the Device Update Azure Portal interface, then expanding the "View device health" section.

## Initiate a Device Sync operation

Device Sync should be triggered if a device has been registered in IoT Hub but is not showing up in Device Update nor in Agent Check results. Device Sync operations can be initiated by navigating to the Diagnostics tab of the DEvice Update Azure Portal interface, expanding the "View device health" section, then clicking "Start a device sync".

Only one Device Sync operation may be active at a time for each Device Update instance.

## Next steps

To learn more about Device Update's diagnostic capabilities, see [Device update diagnostic feature overview](device-update-diagnostics.md)
