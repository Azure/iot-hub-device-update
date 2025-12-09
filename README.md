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
* More details on building the agent here: [How to build agent code](./docs/agent-reference/how-to-build-agent-code.md)

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

To install Valgrind:
```sh
# Install from apt (Ubuntu 22.04 or newer recommended)
sudo apt-get install valgrind

# Or use install-deps.sh
./scripts/install-deps.sh --install-valgrind apt

# Build from source (version 3.23.0)
./scripts/install-deps.sh --install-valgrind source
```

Ensure /usr/bin/valgrind is a valid symlink
e.g. `sudo ln -s /opt/valgrind.3.19.0/bin/valgrind /usr/bin/valgrind`

```sh
cd out
ctest -T memcheck
```

Results will be in `out/Testing/Temporary/MemoryChecker.*.log`

#### Run specific tests under valgrind

```sh
cd out
# Run a specific test
ctest -R <test_name> -T memcheck

# Run tests matching a pattern
ctest -R ".*device_properties.*" -T memcheck

# Run with verbose output
ctest -R <test_name> -T memcheck -V
```

#### Run individual test binary directly with valgrind

```sh
# Run with full leak check
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./path/to/test_binary

# With verbose output and log file
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
  --verbose --log-file=valgrind-output.log ./path/to/test_binary

# With child process tracking
valgrind --leak-check=full --trace-children=yes ./path/to/test_binary
```
