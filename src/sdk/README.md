# Azure Device Update SDK

The Azure Device Update (ADU) SDK provides a C ABI for external applications to communicate with the ADU agent service.

## Overview

The SDK enables applications to:
- Check if the ADU agent is actively processing updates
- Determine when it's safe to enter low-power mode (There is a configurable pause interval when it enters Idle state)

The SDK communicates with the ADU agent through named pipes (FIFOs) and provides a simple, synchronous C ABI that handles the cross-proc comms.

## Library Components

- **Static Library**: `libaducsdk.a` - The SDK static lib
- **Header File**: `aducsdk.h` - C ABI declarations
- **pkg-config**: `aducsdk.pc` - Package config for build systems

## API Reference

### Core Functions

```c
/**
 * @brief Gets the ADU IoT Agent service daemon's current status
 * @return Current service status or error code
 */
ADUC_ServiceStatus GetAduServiceStatus(void);

/**
 * @brief Gets human-readable string for status code
 * @param status The status code
 * @return The string representation (do not free the string)
 */
const char* ADUC_ServiceStatusToString(ADUC_ServiceStatus status);
```

### Status Values

#### Normal Operation States
- `ADUC_ServiceStatus_None` - Agent in initial state
- `ADUC_ServiceStatus_Initializing` - Agent starting up
- `ADUC_ServiceStatus_Downloading` - Downloading update content
- `ADUC_ServiceStatus_Installing` - Installing update
- `ADUC_ServiceStatus_Rebooting` - System reboot in progress
- `ADUC_ServiceStatus_Reporting` - Reporting results to IoT Hub
- `ADUC_ServiceStatus_Paused` - Quiet period before idle
- `ADUC_ServiceStatus_Idle` - Ready for new updates

#### Error States (10000+)
- `ADUC_ServiceStatus_ERROR_UnsupportedApiVersion` - SDK/agent version mismatch
- `ADUC_ServiceStatus_ERROR_AgentServiceNotRunning` - Agent service not running
- `ADUC_ServiceStatus_ERROR_AgentServiceBrokenPipe` - Communication failure
- `ADUC_ServiceStatus_ERROR_AgentServicePermission` - Permission denied
- `ADUC_ServiceStatus_ERROR_AgentServiceTimeout` - Request timeout
- `ADUC_ServiceStatus_ERROR_AgentServiceInternal` - Internal agent error
- `ADUC_ServiceStatus_ERROR_Unknown` - Unknown error

## Building the SDK

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt update && sudo apt install build-essential cmake pkgconfig

# CentOS/RHEL/Fedora
sudo yum install gcc cmake pkgconfig  # CentOS/RHEL 7
sudo dnf install gcc cmake pkgconfig  # Fedora/RHEL 8+
```

### Build Instructions

```bash
# Build the entire project (includes SDK)
./scripts/build.sh -c

# Or build just the SDK target
cmake --build out --target aducsdk

# Install system-wide
sudo cmake --build out --target install
```

### Build Configuration

Configure the SDK at build time:

```bash
# Set custom FIFO path (default: /var/lib/adu/api/apireq.fifo)
cmake -DADUC_API_DEFAULT_FIFO_PATH="/custom/path/api/request.fifo" ..
```

## Using the SDK

### Basic Usage

```c
#include <stdio.h>
#include <aduc/aducsdk.h>

int main() {
    ADUC_ServiceStatus status = GetAduServiceStatus();
    const char* statusStr = ADUC_ServiceStatusToString(status);

    printf("ADU Agent Status: %s\\n", statusStr);

    if (status >= ADUC_ServiceStatus_ERROR_UnsupportedApiVersion) {
        printf("Error communicating with agent\\n");
        return 1;
    }

    return 0;
}
```

### Compilation

#### Using pkg-config (Strongly Recommended)

```bash
# Simple compilation
gcc myapp.c $(pkg-config --cflags --libs aducsdk) -o myapp

# With additional flags
gcc -Wall -O2 myapp.c $(pkg-config --cflags --libs aducsdk) -o myapp
```

#### Using CMake

```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(ADUCSDK REQUIRED aducsdk)

add_executable(myapp main.c)
target_include_directories(myapp PRIVATE ${ADUCSDK_INCLUDE_DIRS})
target_link_libraries(myapp ${ADUCSDK_LIBRARIES})
```

#### Manual Compilation

```bash
# If pkg-config is not available
gcc -I/usr/local/include myapp.c -L/usr/local/lib -laducsdk -o myapp
```

### Error Handling

Always check return values for error conditions:

```c
ADUC_ServiceStatus status = GetAduServiceStatus();

if (status >= ADUC_ServiceStatus_ERROR_UnsupportedApiVersion) {
    // Handle error condition
    const char* error_msg = ADUC_ServiceStatusToString(status);
    fprintf(stderr, "ADU communication error: %s\\n", error_msg);

    switch (status) {
        case ADUC_ServiceStatus_ERROR_AgentServiceNotRunning:
            // Agent service is not running - start it or wait
            break;
        case ADUC_ServiceStatus_ERROR_AgentServiceTimeout:
            // Request timed out - agent may be busy
            break;
        case ADUC_ServiceStatus_ERROR_AgentServicePermission:
            // Permission denied - check user/group membership
            break;
        // Handle other error cases...
    }
    return 1;
}

// Process normal status
printf("Agent is operational: %s\\n", ADUC_ServiceStatusToString(status));
```

## Example Applications

The SDK includes several complete example applications in the `examples/` directory demonstrating common usage patterns:

### Simple Status Check (`simple_status_check.c`)

A basic utility to check agent status with error handling and verbose output options.

```bash
# Build and run
make simple_status_check
./simple_status_check --verbose
```
### Building Examples

```bash
# Build all examples
cd examples/
make all

# Build specific example
make simple_status_check

# Clean build artifacts
make clean

# Run interactive demo
./demo.sh
```

All examples include comprehensive error handling, command-line argument parsing, help text, and demonstrate best practices for production use.

## Yocto Integration

For yocto integration, see [README-Yocto-Integration.md](../../src/sdk/README-Yocto-Integration.md)

## Troubleshooting

### Common Issues

**Error: `aducsdk.h: No such file or directory`**
- Install the SDK: `sudo cmake --build out --target install`
- Or use manual include path: `-I/path/to/sdk/inc`

**Error: `AgentServiceNotRunning`**
- Check if agent is running: `systemctl status deviceupdate-agent`
- Start agent: `sudo systemctl start deviceupdate-agent`

**Error: `AgentServicePermission`**
- Check user permissions on `/var/lib/adu/api/`
- Add user to `adu` group: `sudo usermod -a -G adu $USER`

**Error: `AgentServiceTimeout`**
- Agent may be overloaded; try increasing timeout
- Check agent logs: `journalctl -u deviceupdate-agent`
