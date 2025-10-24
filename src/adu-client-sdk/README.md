# Azure Device Update Extension SDK Suite

The Azure Device Update Extension SDK Suite provides modular development kits for building all types of Azure Device Update extensions without requiring the full agent build infrastructure.

## Overview

This SDK suite enables developers to build:

- **Step Handlers**: Custom update content handlers (apt, script, firmware, etc.)
- **Update Manifest Handlers**: Multi-step workflow orchestration
- **Communication Extensions**: Alternative deployment sources (Event Grid, REST APIs)
- **Content Downloaders**: Custom download protocols and methods
- **Download Handlers**: Post-processing and delta reconstruction
- **Logging Extensions**: Custom log destinations and formatting
- **Configuration Extensions**: Advanced configuration management

## Quick Start

### Install SDK Suite

```bash
# Build the complete SDK suite
./scripts/build.sh --sdk-suite

# Or build specific SDK components
./scripts/build.sh --step-handler-sdk --communication-sdk
```

### Basic Usage

```cpp
// Example: Simple step handler
#include <aduc/step_handler_sdk.hpp>

class MyCustomHandler : public ADUC::StepHandler::ContentHandler {
public:
    Result Download(const WorkflowData* workflow) override {
        // Custom download logic
        return Result::Success();
    }

    Result Install(const WorkflowData* workflow) override {
        // Custom install logic
        return Result::Success();
    }

    // ... implement other required methods
};

// Export the handler
extern "C" {
    ADUC_SDK_EXPORT ContentHandler* CreateUpdateContentHandlerExtension(LogLevel logLevel) {
        return new MyCustomHandler();
    }
}
```

## SDK Components

| SDK Component | Purpose | Package Size | Status |
|---------------|---------|--------------|--------|
| **Core SDK** | Shared utilities and types | ~2 MB | ✅ Planned |
| **Step Handler SDK** | Content handler development | ~4 MB | ✅ Planned |
| **Manifest Handler SDK** | Update orchestration | ~3 MB | ✅ Planned |
| **Communication SDK** | Deployment sources | ~3 MB | ✅ Planned |
| **Content Downloader SDK** | Download protocols | ~3 MB | ✅ Planned |
| **Download Handler SDK** | Post-processing | ~2 MB | ✅ Planned |
| **Logging SDK** | Log destinations | ~2 MB | ✅ Planned |
| **Configuration SDK** | Config management | ~2 MB | ✅ Planned |
| **SDK Suite** | Complete suite | ~20 MB | ✅ Planned |

## Architecture

```text
adu-client-sdk/
├── core/                     # Shared core SDK
├── step-handlers/           # Step handler development
├── update-manifest-handlers/ # Multi-step orchestration
├── communication/           # Alternative deployment sources
├── content-downloaders/     # Download protocols
├── download-handlers/       # Post-processing
├── logging/                 # Log destinations
├── configuration/           # Config management
├── tools/                   # Development tools
├── docs/                    # Documentation
└── suite/                   # Unified SDK package
```

## Development Workflow

### 1. Choose SDK Components

```bash
# For step handlers only
./scripts/build.sh --step-handler-sdk

# For communication extensions
./scripts/build.sh --communication-sdk

# For complete extension development
./scripts/build.sh --sdk-suite
```

### 2. Create Extension Project

```bash
# Use provided templates
cp -r step-handlers/examples/template_project my_handler/
cd my_handler/

# Build extension
mkdir build && cd build
cmake ..
make
```

### 3. Test Extension

```bash
# Use SDK test harness
../tools/test_harness/test_extension my_handler.so

# Validate compliance
../tools/extension_validator/validate_extension my_handler.so
```

## Examples

Each SDK component includes:

- **Copy of Existing Extensions**: Exact replicas of current in-tree implementations
- **Simple Examples**: Basic implementations demonstrating patterns
- **Advanced Examples**: Real-world use cases and integrations
- **Template Projects**: Starting points for new extensions

### Available Examples

#### Step Handlers

- `apt_handler/` - Copy of existing APT handler
- `script_handler/` - Copy of existing script handler
- `simulator_handler/` - Copy of existing simulator
- `simple_file_copy/` - Basic file copy example
- `template_project/` - Template for new handlers

#### Communication Extensions

- `iothub_communication/` - Copy of existing IoT Hub communication
- `eventgrid_mqtt/` - Azure Event Grid MQTT integration
- `restful_api/` - RESTful API deployment source
- `message_queue/` - Message queue integration

#### Logging Extensions

- `syslog_destination/` - System log integration
- `cloud_logging/` - Azure Monitor/AWS CloudWatch
- `real_time_streaming/` - Real-time log streaming

## Build Options

### Command Line Options

```bash
# SDK Suite
--sdk-suite                    # Build complete SDK suite
--sdk-only                     # Build only SDK (no agent)

# Individual SDKs
--step-handler-sdk            # Step handler development kit
--manifest-handler-sdk        # Manifest handler development kit
--communication-sdk           # Communication extension kit
--content-downloader-sdk      # Content downloader kit
--download-handler-sdk        # Download handler kit
--logging-sdk                 # Logging extension kit
--configuration-sdk           # Configuration extension kit

# SDK Options
--sdk-examples                # Include example extensions
--sdk-tools                   # Include development tools
--sdk-static                  # Build static libraries
--sdk-copy-existing           # Copy existing extensions as examples
```

### CMake Options

```cmake
# Enable specific SDK components
set(ADUC_BUILD_STEP_HANDLER_SDK ON)
set(ADUC_BUILD_COMMUNICATION_SDK ON)
set(ADUC_SDK_BUILD_EXAMPLES ON)
```

## Installation

### Package Manager Integration

```bash
# Ubuntu/Debian
sudo apt install adu-step-handler-sdk
sudo apt install adu-extension-sdk-suite

# CentOS/RHEL
sudo yum install adu-step-handler-sdk
sudo yum install adu-extension-sdk-suite

# vcpkg (Windows/Linux)
vcpkg install adu-extension-sdk-suite
```

### Manual Installation

```bash
# Build and install
./scripts/build.sh --sdk-suite
sudo cmake --build out --target install

# Use pkg-config
pkg-config --cflags --libs adu-step-handler-sdk
```

## Documentation

- **[Getting Started Guide](docs/getting_started.md)** - Quick start for each SDK type
- **[API Reference](docs/api_reference/)** - Complete API documentation
- **[Developer Guides](docs/developer_guides/)** - Step-by-step development instructions
- **[Best Practices](docs/best_practices/)** - Design patterns and recommendations
- **[Migration Guides](docs/migration_guides/)** - Porting existing extensions
- **[Troubleshooting](docs/troubleshooting/)** - Common issues and solutions

## Requirements

### System Requirements

- **Linux**: Ubuntu 18.04+, Debian 10+, CentOS 7+
- **Architecture**: x64, ARM32, ARM64
- **Memory**: 64MB RAM for SDK, additional for extensions
- **Storage**: 50MB for individual SDKs, 150MB for complete suite

### Development Requirements

- **Compiler**: GCC 7.4+ or Clang 6.0+
- **CMake**: 3.5 or later
- **Build Tools**: make, pkg-config
- **Optional**: Docker (for containerized development)

## Support

- **Documentation**: [ADU Extension SDK Documentation](docs/)
- **Examples**: See `examples/` directory in each SDK component
- **Issues**: [GitHub Issues](https://github.com/Azure/iot-hub-device-update/issues)
- **Discussions**: [GitHub Discussions](https://github.com/Azure/iot-hub-device-update/discussions)

## Contributing

See [CONTRIBUTING.md](../../CONTRIBUTING.md) for guidelines on contributing to the SDK suite.

## License

This project is licensed under the MIT License - see the [LICENSE](../../LICENSE) file for details.
