# This pre-release is confidential

Access to this pre-release software is confidential and should not be disclosed or shared
with other parties per your NDA with Microsoft. As part of running its service, Azure Device
Update will provide download URLs and APIs to customers which contain the account and instance
name. Because the Azure Device Update service relies on publicly discoverable DNS endpoints,
be sure to avoid entering any sensitive or personally identifiable information.

# Azure Device Update Preview Documentation and Reference Client

| Build              | Status |
|------------------- |--------|
| Ubuntu 18.04 AMD64 | [![Ubuntu 18.04 Build Status](https://dev.azure.com/azure-device-update/adu-linux-client/_apis/build/status/Azure.adu-private-preview?branchName=master)](https://dev.azure.com/azure-device-update/adu-linux-client/_build/latest?definitionId=13&branchName=master) |
| Yocto Warrior Raspberry Pi 3 B+ | [![Raspberry Pi Yocto Build Status](https://dev.azure.com/azure-device-update/adu-linux-client/_apis/build/status/adu-yocto-build?branchName=master)](https://dev.azure.com/azure-device-update/adu-linux-client/_build/latest?definitionId=4&branchName=master) |

This repository contains the following:

* Overview of Azure Device Update.
* API reference for publishing to Azure Device Update.
* API reference for managing updates through Azure Device Update.
* Reference Azure Device Update Agent code.

## What is Azure Device Update?

Azure Device Update is a managed service, that enables you to enable over the air updates
(OTA) for your IoT devices. ADU lets you focus on your solution, by ensuring your devices
are up to date with the latest security fixes or application updates without having to
build and maintain your own update solution. ADU is a reliable and scalable solution that
is based on the Windows Update platform, and integrated with Azure IoT Hub to support devices
globally. ADU also provides controls on how to manage the deployment updates so you are always
in control of when and how devices are updated. ADU also provides reporting capabilities so you
are always up to date on the state of your devices via integration with IoT Hub.

## Getting Started

[Getting Started with Azure Device Update](docs/adu-overview.md)
