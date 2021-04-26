# How To Build the Device Update for IoT Hub Agent

## Dependencies of Device Update Agent

### Required Dependencies

* Azure IoT C SDK
* Delivery Optimization SDK

### Azure IoT C SDK

Use the [Azure IoT C
SDK](https://github.com/Azure/azure-iot-sdk-c)
to connect to IoT Hub and call Azure IoT Plug and Play APIs.

### Delivery Optimization

The [Delivery Optimization
SDK](https://github.com/microsoft/do-client)
SDK provides a robust way for the client to download an update.

## Building the Device Update Agent for Linux

### Installing Dependencies

Use the [scripts/install-deps.sh](../../scripts/install-deps.sh) Linux shell
script for a convenient way to install the dependencies of the Device Update for IoT Hub agent for most use cases.

**Note**: You may be prompted for sudo password or GitHub username and password
when running `install-deps.sh`. If your GitHub account has two factor auth
enabled, use a personal access token (PAT) as the password.

To install all dependencies run:

```shell
./scripts/install-deps.sh -a
```

**Note:**: `--use-ssh` can be used to clone dependencies from the Git repo using SSH instead of https.

To install only the dependencies necessary for the simulator agent:

```shell
./scripts/install-deps.sh --install-aduc-deps -p
```

`install-deps.sh` also provides several options for installing individual
dependencies.  To see the usage info:

```shell
./scripts/install-deps.sh -h
```

### Device Update Linux Build System

The Device Update for IoT Hub reference agent code utilizes CMake for building. An example build script is provided at [scripts/build.sh](../../scripts/build.sh).

#### Build Using build.sh

To build the simulator agent:

```shell
./scripts/build.sh -c
```

To build the reference agent integrated with Delivery Optimization for
downloads but still mocks the `Install` and `Apply` actions:

```shell
./scripts/build.sh -c -p linux
```

To build the agent package for package based updating via APT
Manifest:

```shell
./scripts/build.sh -c -p linux --content-handlers microsoft/apt --build-packages
```
To build the agent that supports provisioning using [IoT Identity Service](https://github.com/Azure/iot-identity-service/blob/main/docs/packaging.md#installing-and-configuring-the-package) specify '--provision-with-iotedge' as shown below:  
Learn more about [how to provision Device Update agent with IoT Identity Service](https://docs.microsoft.com/azure/iot-hub-device-update/device-update-agent-provisioning#how-to-provision-the-device-update-agent-as-a-module-identity). 

```shell
./scripts/build.sh -c -p linux --content-handlers microsoft/apt --provision-with-iotedge
```

To see additional build options with build.sh:

```shell
build.sh -h
```

### Build the agent using CMake

Alternatively, you can build using CMake directly. Set the required product values
for ADUC_DEVICEINFO_MANUFACTURER and ADUC_DEVICEINFO_MODEL in the top-level
[CMakeLists.txt](../../CMakeLists.txt) before building.  Optional CMake values can be found there as well.

```shell
mkdir -p build && pushd build
cmake ..
cmake --build .
popd > /dev/null
```

or using Ninja

```shell
mkdir -p build && pushd build
cmake -G Ninja ..
ninja
popd > /dev/null
```

## Install the Device Update Agent

To install the Device Update Agent after building:

```shell
sudo cmake --build out --target install
```

or using Ninja

```shell
pushd out > /dev/null
sudo ninja install
popd > /dev/null
```

**Note** If the Device Update Agent was built as a daemon, the install targets will install and register the Device Update Agent as a daemon.
