# Device Update for IoT Hub

Device Update for IoT Hub is an end-to-end platform for deploying over-the-air updates (OTA) to your IoT devices—from tiny sensors to gateway-level devices.

## Key Features

* **Integrated Management** - Update management UX integrated with Azure IoT Hub
* **Controlled Rollout** - Gradual update deployment through device grouping and scheduling
* **Flexible APIs** - Programmatic APIs for automation and custom portal experiences
* **Fleet Visibility** - At-a-glance compliance and status views across heterogeneous device fleets
* **Resilient Updates** - Support for A/B updates with seamless rollback
* **Access Control** - Subscription and role-based access controls via Azure portal
* **Offline Support** - On-premise content cache and Nested Edge support for disconnected devices
* **Comprehensive Reporting** - Detailed update management and reporting tools

## Supported Platforms

| Platform           | Status | Notes |
|------------------- |--------|-------|
| Ubuntu 20.04 AMD64 | ✓ Supported | With Delivery Optimization |
| Ubuntu 22.04 AMD64 | [![Ubuntu 22.04 Build Status](https://dev.azure.com/azure-device-update/adu-linux-client/_apis/build/status/Azure.iot-hub-device-update?branchName=main)](https://dev.azure.com/azure-device-update/adu-linux-client/_build/latest?definitionId=27&branchName=main)| Primary development platform |
| Ubuntu 24.04 AMD64 | ✓ Supported | curl downloader only (DO not available) |
| Debian 11 (Bullseye) AMD64 | ✓ Supported | With Delivery Optimization |
| Debian 12 (Bookworm) AMD64 | ✓ Supported | With Delivery Optimization |

## Quick Start

For users familiar with the build process:

```sh
./scripts/install-deps.sh -a
./scripts/build.sh -c -u --build-packages
sudo apt install ./out/deviceupdate-agent_*.deb
```

> **Note**: For detailed build instructions, troubleshooting, and advanced options, see [How to Build the Agent](./docs/agent-reference/how-to-build-agent-code.md)

## Documentation

* **[Device Update for IoT Hub Documentation](https://aka.ms/iot-hub-device-update-docs)** - Official service documentation
* **[Getting Started with the Agent](./docs/agent-reference)** - Agent overview and reference
* **[Building the Agent](./docs/agent-reference/how-to-build-agent-code.md)** - Detailed build instructions
* **[Running the Agent](./docs/agent-reference/how-to-run-agent.md)** - Deployment and configuration
* **[Troubleshooting Guide](./docs/how-to-troubleshoot-guide.md)** - Common issues and solutions

## Contributing

This project welcomes contributions and suggestions. See [CONTRIBUTING.md](./CONTRIBUTING.md) for details.

## Security

See [SECURITY.md](./SECURITY.md) for information on reporting security issues.

## License

This project is licensed under the MIT License - see [LICENSE](./LICENSE) for details.

## Support

For support options, see [SUPPORT.md](./SUPPORT.md).

