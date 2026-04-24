# Building with Delta Handler Support

This document describes how to build the Azure IoT Hub Device Update Agent with the optional Microsoft Delta Download Handler.

## Overview

The Microsoft Delta Download Handler enables differential updates using the [iot-hub-device-update-delta](https://github.com/Azure/iot-hub-device-update-delta) library. This feature is optional and can be enabled during the build process.

## Prerequisites

- All standard ADU Agent build prerequisites
- iot-hub-device-update-delta library installed (see below)

## Installing Delta Library

The delta library is installed as a build dependency using the `install-deps.sh` script:

```bash
# Install only the delta library
./scripts/install-deps.sh --install-delta

# Or install with all dependencies
./scripts/install-deps.sh -a --install-delta

# Specify a different branch/tag (default is 'main')
./scripts/install-deps.sh --install-delta --delta-ref v1.0.0
```

The script will:
- Clone the iot-hub-device-update-delta repository from GitHub
- Build the library
- Install it to system paths (e.g., `/usr/local/lib`, `/usr/local/include`)

## Build Instructions

### Option 1: Using Build Script (Recommended)

```bash
# Install dependencies including delta library
./scripts/install-deps.sh -a --install-delta

# Build with delta handler enabled
./scripts/build.sh -c -t Release -u --build-packages -- -DADUC_BUILD_DELTA_HANDLER=ON
```

### Option 2: Manual CMake Build

```bash
# Install dependencies including delta library
./scripts/install-deps.sh -a --install-delta

# Configure with delta handler enabled
cmake -B out/build/release -S . \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DADUC_BUILD_DELTA_HANDLER=ON \
  -DADUC_BUILD_PACKAGES=ON

# Build
cmake --build out/build/release
```

## How It Works

1. **Install Delta Library**: The `install-deps.sh` script builds and installs the delta library to system paths

2. **Find Module**: The `FindAzureIotHubDeviceUpdateDelta.cmake` module locates the library in:
   - System paths (`/usr/local/lib`, `/usr/local/include`)
   - Custom paths specified via `CMAKE_PREFIX_PATH`

3. **Conditional Compilation**: When `ADUC_BUILD_DELTA_HANDLER=ON`, the delta handler is built and linked against the installed library

4. **Separate Package**: Creates `deviceupdate-agent-delta-handler_<version>.deb` package

## Package Installation

### Install Main Agent First
```bash
sudo apt install ./deviceupdate-agent_<version>.deb
```

### Then Install Delta Handler
```bash
sudo apt install ./deviceupdate-agent-delta-handler_<version>.deb
```

The delta handler package:
- Depends on the main `deviceupdate-agent` package
- Installs the delta handler extension to `/var/lib/adu/extensions/sources/`
- Installs delta library dependencies to `/usr/lib/adu-delta/`
- Automatically registers the handler with the ADU agent
- Configures library paths via `/etc/ld.so.conf.d/adu-delta.conf`

## Package Uninstallation

```bash
# Remove delta handler (keeps main agent)
sudo apt remove deviceupdate-agent-delta-handler

# Or purge to remove configuration files
sudo apt purge deviceupdate-agent-delta-handler
```

The uninstallation:
- Unregisters the delta download handler extension
- Removes the handler library
- Cleans up delta library dependencies
- Removes library path configuration

## Directory Structure

```
.deps/iot-hub-device-update-delta/    # Delta library build output
├── include/                           # Delta library headers
│   └── adudiffapi.h
└── lib/                              # Delta library binaries
    └── libadudiffapi.so

/usr/lib/adu-delta/                   # Installed delta libraries (in package)
└── libadudiffapi.so*

/var/lib/adu/extensions/sources/      # Installed handler
└── libmicrosoft_delta_download_handler.so
```

## CMake Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `ADUC_BUILD_DELTA_HANDLER` | `OFF` | Enable building the delta download handler |
| `DELTA_LIB_TAG` | `main` | Git branch/tag for iot-hub-device-update-delta |
| `DELTA_LIB_INSTALL_DIR` | `.deps/iot-hub-device-update-delta` | Installation directory for delta library |

## Customization

### Using a Different Delta Library Version

```bash
cmake -B build -S . \
  -DADUC_BUILD_DELTA_HANDLER=ON \
  -DDELTA_LIB_TAG=v1.0.0
```

### Using Pre-installed Delta Library

If the delta library is already installed on your system:

```bash
# The Find module will locate it in standard system paths
cmake -B build -S . \
  -DADUC_BUILD_DELTA_HANDLER=ON \
  -DCMAKE_PREFIX_PATH=/path/to/delta/install
```

## Troubleshooting

### Delta Library Build Fails

```bash
# Clean and rebuild
rm -rf build/.deps .deps
cmake -B build -S . -DADUC_BUILD_DELTA_HANDLER=ON
```

### Handler Not Registered

```bash
# Manually register
sudo /usr/bin/AducIotAgent -l 2 \
  --extension-type downloadHandler \
  --extension-id "microsoft/delta:1" \
  --register-extension /var/lib/adu/extensions/sources/libmicrosoft_delta_download_handler.so

# Restart agent
sudo systemctl restart deviceupdate-agent
```

### Library Not Found at Runtime

```bash
# Update library cache
sudo ldconfig

# Check library path
ldd /var/lib/adu/extensions/sources/libmicrosoft_delta_download_handler.so
```

## Development

### Building Only the Delta Handler

```bash
cmake --build build --target microsoft_delta_download_handler
```

### Running Tests

```bash
cd build
ctest -R delta
```

## References

- [iot-hub-device-update-delta GitHub Repository](https://github.com/Azure/iot-hub-device-update-delta)
- [ADU Agent Documentation](../README.md)
- [Extension Development Guide](../docs/agent-reference/how-to-implement-custom-update-handler.md)
