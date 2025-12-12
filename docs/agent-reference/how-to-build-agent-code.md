# How To Build the Device Update Agent

This guide provides detailed instructions for building the Device Update for IoT Hub agent from source.

## Table of Contents

-   [Supported Platforms](#supported-platforms)
-   [Build Dependencies](#build-dependencies)
-   [Runtime Dependencies](#runtime-dependencies)
-   [Quick Start](#quick-start)
-   [Building the Agent](#building-the-device-update-agent-for-linux)
-   [Building Dependencies from Source](#building-dependencies-from-source)
-   [Testing](#build-and-run-the-unit-tests)
-   [Packaging](#build-the-debian-package)
-   [Installation](#install-the-device-update-agent)
-   [Platform-Specific Build Notes](#platform-specific-build-notes)
-   [Advanced Options](#build-options-for-mqtt-and-mqtt-over-websockets-iothub-transport-providers)
-   [Troubleshooting](#troubleshooting)

## Supported Platforms

The Device Update agent has been tested and verified on the following platforms:

| OS Distribution | Version | Architecture | Status | Build Status | Notes |
|----------------|---------|--------------|--------|--------------|-------|
| Ubuntu | 20.04 LTS | AMD64 | ✓ Supported | - | With Delivery Optimization |
| Ubuntu | 22.04 LTS | AMD64 | ✓ Fully Supported | [![Build Status](https://dev.azure.com/azure-device-update/adu-linux-client/_apis/build/status/Azure.iot-hub-device-update?branchName=main)](https://dev.azure.com/azure-device-update/adu-linux-client/_build/latest?definitionId=27&branchName=main) | Primary development platform |
| Ubuntu | 24.04 LTS | AMD64 | ✓ Fully Supported | - | curl downloader only, [see notes](#ubuntu-2404-lts-specifics) |
| Ubuntu | 18.04 LTS | AMD64 | ✗ Not Supported | - | End of support |
| Debian | 11 (Bullseye) | AMD64 | ✓ Supported | - | With Delivery Optimization |
| Debian | 12 (Bookworm) | AMD64 | ✓ Supported | - | With Delivery Optimization |
| Debian | 10 (Buster) | AMD64 | ✗ Not Supported | - | End of support |

## Build Dependencies

The following table lists all build-time dependencies with their Last Known Good (LKG) versions and platform availability.

**Note:** Dependencies marked with an asterisk (*) require special attention. Click the link for details about patches, limitations, or special build instructions.

### Core Build Tools

| Tool | Minimum Version | Ubuntu 20.04 | Ubuntu 22.04 | Ubuntu 24.04 | Debian 11 | Debian 12 | Source |
|------|----------------|--------------|--------------|--------------|-----------|-----------|--------|
| GCC | 10.0 | gcc-10 | gcc-10/11 | gcc-13 | gcc-10 | gcc-12 | System package |
| CMake | 3.10 | 3.16.3 | 3.22.1 | 3.28.3 | 3.18.4 | 3.25.1 | System package or [built from source](#installing-dependencies) |
| Ninja | 1.10+ | 1.10.0 | 1.10.2 | 1.11.1 | 1.10.1 | 1.11.1 | System package |
| Git | 2.0+ | 2.25.1 | 2.34.1 | 2.43.0 | 2.30.2 | 2.39.2 | System package |
| pkg-config | - | ✓ | ✓ | ✓ | ✓ | ✓ | System package |

### Core Dependencies Built from Source

| Dependency | LKG Version/Tag | Ubuntu 20.04 | Ubuntu 22.04 | Ubuntu 24.04 | Debian 11 | Debian 12 | Notes |
|------------|----------------|--------------|--------------|--------------|-----------|-----------|-------|
| Azure IoT C SDK | `LTS_08_2023` | ✓ | ✓ | ✓ | ✓ | ✓ | [Build instructions](#building-azure-iot-c-sdk) |
| Azure Storage SDK for C++ | `azure-core_1.6.0` | ✓ | ✓ | ✓* | ✓ | ✓* | [*Requires GCC 12+ patch](#building-azure-storage-sdk-for-c) |
| Delivery Optimization SDK | `main` (latest) | ✓ | ✓ | ✗ | ✓ | ✓ | [Not available on Ubuntu 24.04](#building-delivery-optimization-sdk) |
| Catch2 | `v3.8.0` | ✓ | ✓ | ✓ | ✓ | ✓ | [Build instructions](#building-catch2) (unit tests only) |
| Parson | Latest | ✓ | ✓ | ✓ | ✓ | ✓ | [Build instructions](#building-parson) |
| Microsoft Delta Download Handler | Submodule | ✓ | ✓ | ✓ | ✓ | ✓ | Built with agent |

### System Package Dependencies

| Package | Purpose | Ubuntu 20.04 | Ubuntu 22.04 | Ubuntu 24.04 | Debian 11 | Debian 12 |
|---------|---------|--------------|--------------|--------------|-----------|-----------|
| libcurl4-openssl-dev | HTTP/HTTPS client | ✓ Required | ✓ Required | ✓ Required | ✓ Required | ✓ Required |
| libssl-dev | TLS/Crypto | ✓ (1.1.1) | ✓ (3.0) | ✓ (3.0) | ✓ (1.1.1) | ✓ (3.0) |
| uuid-dev | UUID generation | ✓ | ✓ | ✓ | ✓ | ✓ |
| zlib1g-dev | Compression | ✓ | ✓ | ✓ | ✓ | ✓ |

### Optional Development Tools

| Tool | Purpose | Installation |
|------|---------|--------------|
| clang-format | Code formatting | `sudo apt install clang-format` |
| cmake-format | CMake formatting | `sudo apt install python3-pip && sudo pip3 install cmake-format` |
| valgrind | Memory testing | `sudo apt install valgrind` (3.19+ recommended) |
| shellcheck | Shell script linting | Via install-deps.sh |

## Runtime Dependencies

The following dependencies are required to run the Device Update agent on deployed devices.

### Core Runtime Requirements

| Dependency | Minimum Version | Package Name (Ubuntu) | Purpose |
|------------|----------------|----------------------|---------|
| systemd | 237+ | systemd | Daemon management |
| libssl | 1.1+ | libssl3 (22.04+), libssl1.1 (20.04) | TLS/crypto operations |
| libcurl | 7.58+ | libcurl4 | HTTP client |
| curl (binary) | 7.58+ | curl | Content download (24.04), rootkey download |

### Platform-Specific Runtime Dependencies

**Ubuntu 20.04, 22.04, Debian 11, Debian 12:**
- `deliveryoptimization-agent` >= 1.0.0 (primary downloader)
- `libdeliveryoptimization` >= 1.0.0
- `curl` (fallback downloader)

**Ubuntu 24.04:**
- `curl` (primary downloader)
- Note: Delivery Optimization not available

## Quick Start

For most users, building the agent is straightforward:

```sh
# Install all dependencies
./scripts/install-deps.sh -a

# Build the agent with unit tests and create Debian package
./scripts/build.sh -c -u --build-packages

# Install the package
sudo apt install ./out/deviceupdate-agent_*.deb
```

For incremental builds after the initial build:

```sh
cd out
ninja
```

To run tests:

```sh
cd out
ctest
# or
ninja test
```

## Building Dependencies from Source

The `install-deps.sh` script automates building dependencies, but this section documents the process, required versions, and any patches needed for each dependency.

### Building Azure IoT C SDK

**Repository:** [https://github.com/Azure/azure-iot-sdk-c](https://github.com/Azure/azure-iot-sdk-c)
**LKG Version:** `LTS_08_2023` branch
**Build Location:** `.workspace/azure-iot-sdk-c`
**Platforms:** All supported (Ubuntu 20.04, 22.04, 24.04, Debian 11, 12)

The Azure IoT C SDK provides the connectivity layer to Azure IoT Hub and implements the Azure IoT Plug and Play APIs.

**Patches Required:** None

**Build Options:**
- MQTT transport (`use_mqtt=ON`)
- MQTT over WebSockets (`use_wsio=ON`)
- Both are built by default

**Known Issues:** None

---

### Building Azure Storage SDK for C++

**Repository:** [https://github.com/Azure/azure-sdk-for-cpp](https://github.com/Azure/azure-sdk-for-cpp)
**LKG Version:** `azure-core_1.6.0` tag
**Build Location:** `.workspace/azure_storage_sdk_dir`
**Platforms:** All supported (Ubuntu 20.04, 22.04, 24.04*, Debian 11, 12*)

The Azure Storage SDK for C++ provides blob storage operations for file uploads.

#### GCC 12+ Compatibility Patch

**Affected Platforms:** Platforms using GCC 12 or later
- Ubuntu 24.04 (GCC 13)
- Debian 12 (GCC 12)
- Any custom build environment with GCC 12+

**Issue:** GCC 12 and later removed implicit standard library includes that previous versions provided. The Azure Storage SDK compilation fails with errors like:
- `error: 'uint8_t' does not name a type`
- `error: 'uint16_t' was not declared in this scope`

**Patch File:** `scripts/patches/azure-storage-sdk-base64-cstdint.patch`

**Affected Files:**
- `sdk/core/azure-core/inc/azure/core/internal/cryptography/base64.hpp`
- `sdk/core/azure-core/src/cryptography/base64.cpp`
- `sdk/core/azure-core/inc/azure/core/uuid.hpp`

**Solution:** The patch adds explicit `#include <cstdint>` directives to the affected files.

**Automatic Application:**
The `install-deps.sh` script automatically detects the GCC version during the Azure Storage SDK build process. If GCC version is 12 or later, the patch is applied automatically. Platforms with GCC 10 or 11 do not require or receive this patch.

**Manual Application:**
```sh
cd .workspace/azure_storage_sdk_dir
git apply ../../scripts/patches/azure-storage-sdk-base64-cstdint.patch
```

**Verification:**
After patching, the SDK compiles successfully with GCC 13 without warnings or errors.

---

### Building Delivery Optimization SDK

**Repository:** [https://github.com/microsoft/do-client](https://github.com/microsoft/do-client)
**LKG Version:** `main` branch (latest)
**Build Location:** `.workspace/do`
**Platforms:** Ubuntu 20.04, 22.04, Debian 11, 12

The Delivery Optimization SDK provides robust, peer-to-peer content distribution for update downloads.

**Availability:**
- ✓ **Ubuntu 20.04:** Fully supported
- ✓ **Ubuntu 22.04:** Fully supported
- ✓ **Debian 11:** Fully supported
- ✓ **Debian 12:** Fully supported
- ✗ **Ubuntu 24.04:** **NOT AVAILABLE** - Package not maintained for this version

**Alternative on Ubuntu 24.04:**
The agent automatically uses the curl content downloader (`libcurl_content_downloader.so`) as a replacement. The build system detects Ubuntu 24.04 and configures accordingly:
- Sets `ADUC_BUILD_WITH_DELIVERY_OPTIMIZATION=OFF`
- Registers curl downloader as primary content downloader
- Package dependencies exclude DO packages

**Build Command (Ubuntu 20.04/22.04, Debian 11/12):**
```sh
./scripts/install-deps.sh --install-do
```

**Patches Required:** None

**Known Limitations on Ubuntu 24.04:**
- No peer-to-peer download optimization
- All content downloads via direct HTTP/HTTPS using curl
- Functionally equivalent but may use more bandwidth in fleet scenarios

---

### Building Catch2

**Repository:** [https://github.com/catchorg/Catch2](https://github.com/catchorg/Catch2)
**LKG Version:** `v3.8.0` tag
**Build Location:** `.workspace/catch2`
**Platforms:** All supported (Ubuntu 20.04, 22.04, 24.04, Debian 11, 12)
**Purpose:** Unit testing framework

**Patches Required:** None

**Note:** Only required for building and running unit tests. Not needed for production builds.

---

### Building Parson

**Repository:** [https://github.com/kgabis/parson](https://github.com/kgabis/parson)
**LKG Version:** Latest from master
**Build Location:** `.workspace/parson`
**Platforms:** All supported (Ubuntu 20.04, 22.04, 24.04, Debian 11, 12)
**Purpose:** Lightweight JSON parser

**Patches Required:** None

**Known Issues:** None

## Building the Device Update Agent for Linux

### Installing Dependencies

Use the [scripts/install-deps.sh](../../scripts/install-deps.sh) Linux shell
script for a convenient way to install the dependencies of the Device Update for IoT Hub agent for most use cases.

**Note**: You may be prompted for sudo password or GitHub username and password
when running `install-deps.sh`. If your GitHub account has two factor auth
enabled, use a personal access token (PAT) as the password.

To install all dependencies run:

```sh
./scripts/install-deps.sh -a
```

**Note:**: `--use-ssh` can be used to clone dependencies from the Git repo using SSH instead of https.

To install only the dependencies necessary for the agent:

```sh
./scripts/install-deps.sh --install-aduc-deps --install-packages --install-do
```

`install-deps.sh` also provides several options for installing individual
dependencies. To see the usage info:

```sh
./scripts/install-deps.sh -h
```

### Install Optional Development Tools

- Install the clang-format package (required for running `scripts/clang-format.sh`):

```sh
sudo apt install clang-format
```

- Install pip3 and cmake-format (required for running `scripts/cmake-format.sh`):

```sh
sudo apt install python3-pip
sudo --set-home pip3 install cmake-format
```

### Device Update Linux Build System

The Device Update for IoT Hub reference agent code utilizes CMake for building. An example build script is provided at [scripts/build.sh](../../scripts/build.sh).

#### Build Using build.sh

To build the reference agent with the default parameters:

```sh
./scripts/build.sh -c
```

To see additional build options with build.sh:

```sh
build.sh -h
```

### Build and Run the unit tests

To build and run the unit tests:

```sh
./scripts/build.sh -c -u
pushd out
ctest # or ninja test
```

For more test run options:

```sh
ctest -h
```

### Run the Unit Tests under Valgrind

The current supported valgrind versions are 3.15+ (Ubuntu 20.04), 3.18+ (Ubuntu 22.04), or 3.23.0 from source.

**Installing Valgrind:**

```sh
# Automatic installation (recommended) - uses apt on Ubuntu 20.04+
./scripts/install-deps.sh --install-valgrind auto

# Install from apt package manager
./scripts/install-deps.sh --install-valgrind apt

# Build from source (version 3.23.0)
./scripts/install-deps.sh --install-valgrind source

# Skip installation
./scripts/install-deps.sh --install-valgrind skip
```

**Running Tests:**

There is a top-level `DartConfiguration.tcl` in the source tree that contains valgrind path and arguments.
Running memcheck with following command will result in CTest generating a `DartConfiguration.tcl` under the `out` dir and running all the tests using valgrind:

```sh
cd out
ctest -T memcheck
```

No suppression file is currently used, so the goal is for all the unit tests to run valgrind-clean and to fix even the false-positives.

#### Advanced Valgrind Usage

**Run specific tests:**
```sh
cd out
# Run a single test
ctest -R device_properties_ut -T memcheck

# Run tests matching a pattern with verbose output
ctest -R ".*config_utils.*" -T memcheck -V

# Run tests and continue on failure
ctest -T memcheck --output-on-failure
```

**View detailed results:**
```sh
# View the most recent memcheck log
cat out/Testing/Temporary/MemoryChecker.*.log

# List all memcheck logs
ls -lt out/Testing/Temporary/MemoryChecker.*.log

# Search for leaks in logs
grep -i "definitely lost\|indirectly lost" out/Testing/Temporary/MemoryChecker.*.log
```

**Run test binaries directly with Valgrind:**
```sh
# Full leak check with origins
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
  --verbose --log-file=valgrind-mytest.log \
  ./src/agent/adu_core_interface/tests/device_properties_ut

# With child process tracking (useful for tests that spawn processes)
valgrind --leak-check=full --trace-children=yes \
  --child-silent-after-fork=yes \
  ./path/to/test_binary

# Generate detailed XML reports
valgrind --leak-check=full --xml=yes --xml-file=valgrind-report.xml \
  ./path/to/test_binary
```

**Common Valgrind options:**
- `--leak-check=full` - Show detailed information about each leak
- `--show-leak-kinds=all` - Show all types of leaks (definite, indirect, possible, reachable)
- `--track-origins=yes` - Track origins of uninitialized values
- `--verbose` - More detailed output
- `--log-file=<file>` - Save output to file
- `--suppressions=<file>` - Use suppression file for known false positives
- `--gen-suppressions=all` - Generate suppression entries for reported errors

**Tips for memory leak testing:**
1. Always run tests in a clean build to ensure accurate results
2. Use `--track-origins=yes` to find where uninitialized values come from
3. Save logs to files for easier analysis: `--log-file=valgrind-%p.log` (where %p is process ID)
4. For CI/CD integration, use `--error-exitcode=1` to fail on errors
5. Create suppression files for external library false positives

### Build the Debian package

To build the debian package (will be output to the `out` directory):

```sh
./scripts/build.sh --build-packages
```

### Build the agent using CMake

Alternatively, you can build using CMake directly. Set the required product values
for ADUC_DEVICEINFO_MANUFACTURER and ADUC_DEVICEINFO_MODEL in the top-level
[CMakeLists.txt](../../CMakeLists.txt) before building. Optional CMake values can be found there as well.

```sh
mkdir -p build && pushd build
cmake ..
cmake --build .
popd > /dev/null
```

or using Ninja

```sh
mkdir -p build && pushd build
cmake -G Ninja ..
ninja
popd > /dev/null
```

You can do incremental builds with Ninja:

```sh
pushd out && ninja
popd
```

### Use Curl for RootKey Package Download instead of Delivery Optimization Agent (DO)

One can already configure the agent to use curl to download update payload content (by registering curl content downloader extension, `/var/lib/adu/extensions/sources/libcurl_content_downloader.so`).

However, to switch to curl for downloading the RootKey Package (infrastructure file for update signature verification), one must currently rebuild the agent from sources by providing the `--rootkeypkg-curl` parameter to [build.sh](../../scripts/build.sh).

For example:

```sh
./scripts/build.sh -c --rootkeypkg-curl
```

Notes:

- Just as with update payload downloads, using curl for downloads will require `/usr/bin/curl` to be available on the device that has the AducIotAgent binary.

- curl will be invoked with the following cmd-line: `/usr/bin/curl -L -C - -o /path/to/output/file`

### Build Options for MQTT and MQTT over WebSockets IotHub Transport Providers

#### Allow MQTT or MQTT over Websockets IotHub Transport Protocols Driven from Config

By Default, both mqtt and mqtt/WebSockets will be linked into the AducIotAgent binary agent and driven by the `"iotHubProtocol"` config property with valid values of `"mqtt"` or `"mqtt/ws"`, for MQTT and MQTT over WebSockets, respectively.

Here is the default in top-level `CMakeLists.txt`:

```sh
set (
    ADUC_IOT_HUB_PROTOCOL
    "IotHub_Protocol_from_Config"
    CACHE
        STRING
        "The protocol for Azure IotHub SDK communication. Options are MQTT, MQTT_over_WebSockets, and IotHub_Protocol_from_Config")
```

Sample `/etc/adu/du-config.json` that selects `MQTT` value for the `iotHubProtocol` property:

```json
{
  ...
  "iotHubProtocol": "mqtt"
}
```

Sample `/etc/adu/du-config.json` that selects `MQTT over WebSockets` value for the `iotHubProtocol` property:

```json
{
  ...
  "iotHubProtocol": "mqtt/ws"
}
```

#### Use only MQTT IotHub Transport Protocol

If using only `MQTT`, then choosing `MQTT` for `ADUC_IOT_HUB_PROTOCOL` in the top-level `CMakeLists.txt` will reduce the size of Type=SizeMinRel AducIotAgent binary by about 60 KB, which is relatively small compared to the overall footprint that is on the order of 2-3 megabytes.

To link in only MQTT transport provider, set this in the top-level `CMakeLists.txt`:

```sh
set (
    ADUC_IOT_HUB_PROTOCOL
    "MQTT"
    CACHE
        STRING
        "The protocol for Azure IotHub SDK communication. Options are MQTT, MQTT_over_WebSockets, and IotHub_Protocol_from_Config")
```

#### Use only MQTT over WebSockets IotHub Transport Protocol

After enabling WebSockets above using install-deps.sh so that it builds libiothub_client_mqtt_ws_transport.a static library, modify the top-level CMakeLists.txt to use MQTT_over_WebSockets:

```sh
set (
    ADUC_IOT_HUB_PROTOCOL
    "MQTT_over_WebSockets"
    CACHE
        STRING
        "The protocol for Azure IotHub SDK communication. Options are MQTT, MQTT_over_WebSockets, and IotHub_Protocol_from_Config")
```

Doing ./build.sh after setting this to `"MQTT_over_WebSockets"` will have the MQTT traffic go over a websocket on port `443`.
Using `"MQTT"` will use SecureMQTT over port `8883`.

Please note that, by default, both MQTT (`libiothub_client_mqtt_transport.a`) and MQTT over WebSockets(`libiothub_client_mqtt_ws_transport.a`) static libraries are built by `install-deps.sh` via the `use_mqtt` and `use_wsio` -D configs.

```sh
# Verify mqtt transport static lib exists
$ locate libiothub_client_mqtt_transport.a | \
    grep '/usr/local/lib/'
/usr/local/lib/libiothub_client_mqtt_transport.a
```

```sh
# Verify mqtt over websockets static lib exists
$ locate libiothub_client_mqtt_ws_transport.a | \
    grep '/usr/local/lib/'
/usr/local/lib/libiothub_client_mqtt_ws_transport.a
```

## Install the Device Update Agent

### Install the Device Update Agent after building

```sh
sudo cmake --build out --target install
```

or using Ninja

```sh
pushd out > /dev/null
sudo ninja install
popd > /dev/null
```

**Note** If the Device Update Agent was built as a daemon, the install targets will install and register the Device Update Agent as a daemon.

### Install the Device Update Agent and Extensions from Debian Package

After building the Debian package using `build.sh --build-packages`, do:

```sh
sudo apt install ./out/{PKG_NAME}.deb
```

## Platform-Specific Build Notes

This section contains important platform-specific information, limitations, and workarounds.

### Ubuntu 20.04 LTS Specifics

**GCC Version:** 10.3.0 (default)
**CMake Version:** 3.16.3 (system), can use newer from install-deps.sh

**Delivery Optimization:**
- Fully supported
- Installed as system packages

**Content Downloader:**
- Primary: Delivery Optimization
- Fallback: curl

**Known Issues:** None

---

### Ubuntu 22.04 LTS Specifics

**GCC Version:** 11.x (default) or 10.x
**CMake Version:** 3.22.1 (system)

**Delivery Optimization:**
- Fully supported
- Primary development and CI/CD platform

**Content Downloader:**
- Primary: Delivery Optimization
- Fallback: curl

**Known Issues:** None

---

### Ubuntu 24.04 LTS Specifics

**GCC Version:** 13.3.0 (system default)
**CMake Version:** 3.28.3 (system)

**Key Changes from Previous Versions:**
- GCC 8/9/10 installation automatically skipped
- Uses system default GCC 13 compiler
- Build system auto-detects Ubuntu 24.04

**Delivery Optimization:**
- **NOT AVAILABLE** - Package not maintained for Ubuntu 24.04
- Build automatically disables DO via `ADUC_BUILD_WITH_DELIVERY_OPTIMIZATION=OFF`
- `build.sh` detects Ubuntu 24.04 and configures accordingly

**Content Downloader:**
- Primary: `libcurl_content_downloader.so`
- Automatically registered during package installation
- No Delivery Optimization fallback available

**Compiler Compatibility:**
- GCC 13 requires Azure Storage SDK patch (applied automatically by install-deps.sh when GCC >= 12)
- See [Building Azure Storage SDK for C++](#building-azure-storage-sdk-for-c) for details

**Package Dependencies:**
- Debian package depends on `curl` only
- No `deliveryoptimization-agent` or `libdeliveryoptimization` dependencies

**Known Limitations:**
- No peer-to-peer download optimization
- All downloads are direct HTTP/HTTPS via curl

---

### Debian 11 (Bullseye) Specifics

**GCC Version:** 10.2.1 (default)
**CMake Version:** 3.18.4 (system)

**Delivery Optimization:**
- Fully supported
- Installed via install-deps.sh

**Content Downloader:**
- Primary: Delivery Optimization
- Fallback: curl

**Known Issues:** None

---

### Debian 12 (Bookworm) Specifics

**GCC Version:** 12.2.0 (default)
**CMake Version:** 3.25.1 (system)

**Delivery Optimization:**
- Fully supported
- Installed via install-deps.sh

**Content Downloader:**
- Primary: Delivery Optimization
- Fallback: curl

**Compiler Compatibility:**
- GCC 12 requires Azure Storage SDK patch (applied automatically by install-deps.sh when GCC >= 12)
- See [Building Azure Storage SDK for C++](#building-azure-storage-sdk-for-c) for details

**Known Issues:** None

---

## Troubleshooting

### Build Failures

#### Azure Storage SDK Compilation Errors (GCC 12+)

If you encounter errors related to `uint8_t` or missing `<cstdint>` includes when building the Azure Storage SDK, ensure you're using the latest version of `install-deps.sh` which includes automatic GCC version detection and patch application for GCC 12+. This affects:
- Ubuntu 24.04 (GCC 13)
- Debian 12 (GCC 12)
- Any custom environment using GCC 12 or later

The patch is only applied when the build system detects GCC version 12 or higher.

#### Permission Issues

If you encounter permission errors with files in `.workspace/`, the build scripts will automatically fix ownership. If issues persist, manually correct ownership:

```sh
sudo chown -R $(id -un):$(id -gn) .workspace/
```

#### Symlink Issues

If cmake or shellcheck symlinks are broken, re-run the install script:

```sh
./scripts/install-deps.sh --install-packages
```

### Valgrind Setup

To run tests under valgrind memcheck, ensure valgrind 3.19.0 or later is installed:

```sh
# Verify valgrind is accessible
which valgrind
# If not found, create symlink to your valgrind installation
sudo ln -s /opt/valgrind.3.19.0/bin/valgrind /usr/bin/valgrind
```

Then run memcheck:

```sh
cd out
ctest -T memcheck
```

Results will be in `out/Testing/Temporary/MemoryChecker.*.log`

## Run Device Update Agent

Run Device Update Agent by following these [instructions](./how-to-run-agent.md)

### Verify MQTT Port

If using MQTT, run the following netstat command to verify that it is connecting to the remote MQTT port of 8883:

```sh
$ sudo ./out/bin/AducIotClient -l0 -e > /dev/null 2>&1 &
$ sudo netstat -pantu | grep Adu
tcp        0      0 <LOCAL IP ADDR>:<LOCAL PORT>       <REMOTE IP ADDR>:8883         ESTABLISHED <PID>/./out/bin/Aduc
$ fg
<ctrl-c>
```

### Verify MQTT over WebSockets Port

If using MQTT over WebSockets, run the following netstat command to verify that it is connecting to the remote WebSockets port of 443:

```sh
$ sudo ./out/bin/AducIotClient -l0 -e > /dev/null 2>&1 &
$ sudo netstat -pantu | grep Adu
tcp        0      0 <LOCAL IP ADDR>:<LOCAL PORT>       <REMOTE IP ADDR>:443         ESTABLISHED <PID>/./out/bin/Aduc
$ fg
<ctrl-c>
```
