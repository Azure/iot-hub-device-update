# Getting Started with Early Access

Welcome to the early access 0.9 release of Device Update for IoT Hub. 

This release will give you a chance to preview some of the upcoming features such as:

* Delta updates allow you to generate a very small update which represents only the changes between two full updates – a source image and a target image. This is ideal for reducing the bandwidth used to download an update to a device, particularly if there have only been a small number of changes between the source and target updates.
* Automatic group provisioning will automatically create groups of devices based on their compatibility properties and device tags, so you can easily deploy updates to your devices without the additional overhead
* Improved troubleshooting features such as agent check and device sync help you troubleshoot and repair your devices with ease
* Automatic rollback enables you to define a fallback version for your managed devices if they meet the rollback criteria that can be easily set from the cloud
* Azure CLI Support enables you to create and mange DU resources, groups and deployments using command line functions
* Support for OS platforms such as Ubuntu 18.04 and Ubuntu 20.04
* and many more….



## Onboarding Requirements

Please fill this [2-min onboarding form](https://aka.ms/aduearlyaccessform) before you start using the early access to ensure that we can set up the right preview environments for you. Once you do this, you can access the [early access Azure Portal here](https://portal.azure.com/?feature.adu=true&feature.canmodifystamps=true&Microsoft_Azure_Iothub=tip1&Microsoft_Azure_ADU_Diagnostic=true)

## Documentation

General information on Device Update for IoT Hub overview as well as concepts can be found [here](https://aka.ms/iot-hub-device-update-docs)

Additional documentation specific to the new features available in the early access can be found in the [docs folder](https://github.com/Azure/iot-hub-device-update/tree/early-access/0.9/docs) of the early-access/0.9 branch

## Device Update Agent

The Device update and delivery optimizationagent build for Ubuntu 18.04 have been provided in [Early Access Agent artifacts](https://github.com/Azure/iot-hub-device-update/tree/early-access/0.9/early-access-agent-artifacts) in addition to the agent code.


## Reporting bugs

The Device Update for IoT Hub team is ready to share with you, engage in discussions, and hear your voices. Your thoughts will make us better, so don't hold back. Bugs
as well as feedback can be filed in the [issues section](https://github.com/Azure/iot-hub-device-update/issues) of the GitHub repo. Please add an "early-access" label to your issues to help us address your feedback in a timely manner. 

## Support policy

Early access builds are offered by Microsoft under [Supplemental Terms of Use for Microsoft Azure Previews.](https://azure.microsoft.com/en-us/support/legal/preview-supplemental-terms/)
By using the early access, you agree to these terms. 

To learn more about Microsoft policies, review the [privacy statement](https://privacy.microsoft.com/en-us/privacystatement)


## Reference agent

| Build              | Status |
|------------------- |--------|
| Ubuntu 18.04 AMD64 | [![Ubuntu 18.04 Build Status](https://dev.azure.com/azure-device-update/adu-linux-client/_apis/build/status/Azure.iot-hub-device-update?branchName=main)](https://dev.azure.com/azure-device-update/adu-linux-client/_build/latest?definitionId=27&branchName=main)|


