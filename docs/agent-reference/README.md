# Device Update for IoT Hub - Reference Agent Documentation

## Overview

Device Update for IoT Hub is an Azure service that enables secure, scalable over-the-air (OTA) updates for IoT devices. This repository contains the **reference implementation** of the Device Update agent — a comprehensive, production-ready agent that demonstrates best practices for integrating Device Update capabilities into IoT devices and applications.

> 📖 **For service documentation and tutorials**, visit [Device Update for IoT Hub on Microsoft Learn](https://learn.microsoft.com/azure/iot-hub-device-update/)

## What's New in 1.3.0

| Feature | Description | Documentation |
|---------|-------------|---------------|
| **X.509 Certificate Auth** | Full client certificate authentication with PKCS#11 and HSM support | [X.509 Authentication Guide](how-to-x509-authentication.md) |
| **Delta Download Handler** | Bandwidth-efficient differential updates using Microsoft Delta | [Building with Delta Handler](building-with-delta-handler.md) |
| **Service Status API** | Cross-process SDK for querying agent state and managing idle/pause | [GetAduServiceStatus SDK](GetAduServiceStatus.md) |
| **Curl Default Downloader** | Curl content downloader as default (required for Ubuntu 24.04, Debian 13) | [Build Guide — Content Downloader](how-to-build-agent-code.md) |
| **Ubuntu 24.04 LTS** | Full support with GCC 13 | [Platform Matrix](how-to-build-agent-code.md#platform-compatibility-matrix) |
| **Debian 13 (Trixie)** | Full support with GCC 12 | [Platform Matrix](how-to-build-agent-code.md#platform-compatibility-matrix) |
| **Detailed Error Reporting** | ExtendedResultCode shown in IoT Hub "Last Attempted Update" details | [Error Codes](device-update-agent-extended-result-codes.md) |
| **Logging Improvements** | Enhanced agent and rootkey workflow logging | — |

## Documentation Structure

### 🎯 Getting Started

| Document | Description |
|----------|-------------|
| [Architecture Overview](architecture-overview.md) | Agent components, communication flow, security model, and update lifecycle |
| [Quick Start Guide](quick-start.md) | Get the agent built and running in 15 minutes |
| [Configuration Guide](configuration-guide.md) | Complete `du-config.json` reference with all connection types |

### 🏗️ Building & Running

| Document | Description |
|----------|-------------|
| [Building the Agent](how-to-build-agent-code.md) | Dependencies, platform support, build options, and CMake configuration |
| [Building with Delta Handler](building-with-delta-handler.md) | Enable the optional Microsoft Delta Download Handler |
| [Running the Agent](how-to-run-agent.md) | Daemon setup, user/group configuration, command-line options |
| [Installing from Packages](how-to-install-deb-pkg-on-rpi.md) | Install pre-built Debian packages on Raspberry Pi |

### 🔐 Authentication & Security

| Document | Description |
|----------|-------------|
| [X.509 Authentication](how-to-x509-authentication.md) | Certificate setup, PKCS#11, HSM integration, and testing |
| [Goal State & Workflow](goal-state-support.md) | Deployment protocol, state machine, and cryptographic validation |

### 🔌 Extensibility

| Document | Description |
|----------|-------------|
| [Extensibility Points](device-update-agent-extensibility-points.md) | Overview of all 5 extension types and first-party plugins |
| [Implementing Custom Handlers](how-to-implement-custom-update-handler.md) | Step-by-step guide for creating custom Step Handlers |
| [Registering Extensions](registering-device-update-extensions.md) | How to register handlers, downloaders, and enumerators |
| [Extension Contract Versions](extension-contract-versions.md) | Versioning scheme for extension compatibility |
| [Multi-Component Updates](multi-component-updating.md) | Proxy updates and multi-step ordered execution |

### 📱 Integration & SDK

| Document | Description |
|----------|-------------|
| [Service Status API (SDK)](GetAduServiceStatus.md) | CrossProc query API, idle pause timer, and SDK examples |
| [Modifying the Agent](how-to-modify-the-agent-code.md) | Customization points for PnP interfaces, handlers, and OS porting |
| [Simulating Update Results](how-to-simulate-update-result.md) | Test deployments without real updates using the simulator handler |

### 🔍 Operations & Troubleshooting

| Document | Description |
|----------|-------------|
| [Troubleshooting Guide](../how-to-troubleshoot-guide.md) | Common issues, log collection, and error code decoding |
| [Diagnostics Log Collection](../diagnostics-log-collection.md) | Remote log collection and Azure Storage upload |
| [Error Codes Reference](device-update-agent-extended-result-codes.md) | ExtendedResultCode encoding, facility codes, and result code generation |
| [Update Manifest v5 Schema](update-manifest-v5-schema.md) | Manifest format for multi-step and proxy updates |
| [Steps Handler (Update Manifest Handler)](steps-handler.md) | How the agent processes multi-step manifests, per-component / per-step flow, reboot propagation, and result codes |

## Quick Navigation

### By Role

- **🔰 New to Device Update**: [Architecture Overview](architecture-overview.md) → [Quick Start](quick-start.md)
- **📱 App Developer**: [Service Status API](GetAduServiceStatus.md) → [Simulating Updates](how-to-simulate-update-result.md)
- **🔧 Device Integrator**: [Building the Agent](how-to-build-agent-code.md) → [X.509 Authentication](how-to-x509-authentication.md)
- **🏗️ Extension Developer**: [Extensibility Points](device-update-agent-extensibility-points.md) → [Implementing Custom Handlers](how-to-implement-custom-update-handler.md)
- **🛠️ DevOps Engineer**: [Running the Agent](how-to-run-agent.md) → [Diagnostics Log Collection](../diagnostics-log-collection.md) → [Troubleshooting](../how-to-troubleshoot-guide.md)

### By Scenario

- **🚀 Quick Evaluation**: [Quick Start Guide](quick-start.md)
- **🏭 Production Deployment**: [Configuration Guide](configuration-guide.md) → [X.509 Authentication](how-to-x509-authentication.md)
- **📦 Delta Updates**: [Building with Delta Handler](building-with-delta-handler.md)
- **🔧 Custom Update Types**: [Implementing Custom Handlers](how-to-implement-custom-update-handler.md) → [Registering Extensions](registering-device-update-extensions.md)
- **🌐 Multi-Component Devices**: [Multi-Component Updates](multi-component-updating.md)

## Key Features by Version

| Feature | v1.1 | v1.2 | v1.3 | Description |
|---------|------|------|------|-------------|
| **Basic Updates** | ✅ | ✅ | ✅ | APT, SWUpdate, Script handlers |
| **Multi-Component** | ✅ | ✅ | ✅ | Proxy updates and component enumeration |
| **Enhanced Security** | ✅ | ✅ | ✅ | Root key package validation |
| **X.509 Auth** | ⚠️ | ✅ | ✅ | Full certificate authentication with PKCS#11 |
| **Service Status API** | ❌ | ❌ | ✅ | CrossProc query SDK with idle pause |
| **Delta Download Handler** | ❌ | ❌ | ✅ | Differential updates via Microsoft Delta |
| **Curl Default Downloader** | ❌ | ❌ | ✅ | Curl as default content downloader |
| **Ubuntu 24.04 / Debian 13** | ❌ | ❌ | ✅ | Latest platform support |
| **Detailed Error Reporting** | ❌ | ❌ | ✅ | ExtendedResultCode in IoT Hub details |

## Prerequisites

### System Requirements

| Requirement | Minimum | Recommended | Notes |
|-------------|---------|-------------|-------|
| **Operating System** | Ubuntu 20.04, Debian 11 | Ubuntu 22.04+, Debian 12+ | [Platform matrix](how-to-build-agent-code.md#platform-compatibility-matrix) |
| **Architecture** | x64, ARM32, ARM64 | x64, ARM64 | ARM64 recommended for ARM deployments |
| **Memory** | 128MB RAM | 256MB RAM | Additional space needed during updates |
| **Storage** | 100MB for agent | 500MB+ | Varies by update content size |
| **Network** | Internet connectivity | Stable broadband | Required for Azure IoT Hub communication |

### Quick Start

```bash
# Clone the repository
git clone https://github.com/Azure/iot-hub-device-update.git
cd iot-hub-device-update

# Install all dependencies
./scripts/install-deps.sh -a

# Build the agent
./scripts/build.sh -c
```

> 📚 See [Building the Agent](how-to-build-agent-code.md) for build options, platform-specific guidance, and cross-compilation.

## Sample Configuration

```json
{
  "schemaVersion": "1.2",
  "aduShellTrustedUsers": ["adu", "do"],
  "iotHubProtocol": "mqtt",
  "agents": [
    {
      "name": "main",
      "runas": "adu",
      "connectionSource": {
        "connectionType": "string",
        "connectionData": "HostName=<hub>.azure-devices.net;DeviceId=<device>;SharedAccessKey=<key>"
      },
      "manufacturer": "Contoso",
      "model": "Smart-Device-v1"
    }
  ]
}
```

> See [Configuration Guide](configuration-guide.md) for all connection types (Connection String, AIS, X.509) and advanced options.

## Getting Help

- **📖 Documentation**: [Microsoft Learn - Device Update](https://learn.microsoft.com/azure/iot-hub-device-update/)
- **🐛 Issues**: [GitHub Issues](https://github.com/Azure/iot-hub-device-update/issues)
- **💬 Discussions**: [GitHub Discussions](https://github.com/Azure/iot-hub-device-update/discussions)
- **📧 Support**: [Azure Support](https://azure.microsoft.com/support/)
- **🤝 Contributing**: [CONTRIBUTING.md](../../CONTRIBUTING.md) · [CODE_OF_CONDUCT.md](../../CODE_OF_CONDUCT.md) · [SECURITY.md](../../SECURITY.md)

---

> **Next Steps**: Start with the [Architecture Overview](architecture-overview.md) to understand the core concepts, then follow the [Quick Start Guide](quick-start.md) to get hands-on.
