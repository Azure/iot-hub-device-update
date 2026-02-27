# What is Device Update for IoT Hub?

Device Update for IoT Hub is a service that enables you to deploy over-the-air updates (OTA) for your IoT devices.

Device Update for IoT Hub is an end-to-end platform that customers can use to publish, distribute, and manage over-the-air updates for everything from tiny sensors to gateway-level devices.

Device Update for IoT Hub also provides controls on how to manage the deployment updates so you are always in control of when and how devices are updated. Device Update for IoT Hub also provides reporting capabilities so you are always up to date on the state of your devices via integration with IoT Hub.

Device Update for IoT Hub features provide a powerful and flexible experience, including:

* Update management UX integrated with Azure IoT Hub
* Gradual update rollout through device grouping and update scheduling controls
* Programmatic APIs to enable automation and custom portal experiences
* At-a-glance update compliance and status views across heterogenous device fleets
* Support for resilient device updates (A/B) to deliver seamless rollback
* Subscription and role-based access controls available through the Azure.com portal
* On-premise content cache and Nested Edge support to enable updating cloud disconnected devices
* Detailed update management and reporting tools

## Reference Agent

While we aim to provide production-ready, high-quality Device Update agent code and installation packages, this project is intended to serve as a **reference implementation** demonstrating how devices can communicate with and receive update deployments from the Azure Device Update cloud service.

We encourage you to extend, enhance, and modify the source code to best fit your specific scenario requirements. If you have ideas, suggestions, or encounter issues, please report them using [GitHub Issues](https://github.com/Azure/iot-hub-device-update/issues) or the [Discussion Forum](https://github.com/Azure/iot-hub-device-update/discussions).

| Build              | Status |
|------------------- |--------|
| Ubuntu 22.04 AMD64 | [![Ubuntu 22.04 Build Status](https://dev.azure.com/azure-device-update/adu-linux-client/_apis/build/status/Azure.iot-hub-device-update?branchName=main)](https://dev.azure.com/azure-device-update/adu-linux-client/_build/latest?definitionId=27&branchName=main)|

## Getting Started

* [Device Update for IoT Hub](https://aka.ms/iot-hub-device-update-docs)
* [Getting Started with Device Update Agent](./docs/agent-reference)
* More details on building the agent here: [How to build agent code](./docs/agent-reference/how-to-build-agent-code.md)
* For remote diagnostics and log collection: [Diagnostics Log Collection](./docs/diagnostics-log-collection.md)

## Contributing

We welcome contributions and feedback! Please see:

* [Contributing Guide](./CONTRIBUTING.md) - How to contribute code or documentation
* [Code of Conduct](./CODE_OF_CONDUCT.md) - Community guidelines
