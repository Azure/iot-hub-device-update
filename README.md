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

## Reference agent

| Build              | Status |
|------------------- |--------|
| Ubuntu 22.04 AMD64 | [![Ubuntu 22.04 Build Status](https://dev.azure.com/azure-device-update/adu-linux-client/_apis/build/status/Azure.iot-hub-device-update?branchName=main)](https://dev.azure.com/azure-device-update/adu-linux-client/_build/latest?definitionId=27&branchName=main)|

## Getting started

* [Device Update for IoT Hub](https://aka.ms/iot-hub-device-update-docs)
* [Getting Started with Device Update Agent](./docs/agent-reference)

## Quick Start

### Build and Install

```sh
./scripts/install-deps.sh -a
./scripts/build.sh -c -u --build-packages
cd out
sudo cmake --build . --target install
```

### Incremental Build

```sh
cd out
ninja
```

### Run Tests

```sh
cd out
ctest
```

or, alternatively:

```sh
ninja test
```

### Run tests under valgrind memcheck

Ensure /usr/bin/valgrind is a valid symlink
e.g. `sudo ln -s /opt/valgrind.3.19.0/bin/valgrind /usr/bin/valgrind`

```sh
cd out
ctest -T memcheck
```

Results will be in `out/Testing/Temporary/MemoryChecker.*.log`
