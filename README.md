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
| Debian 13 (Trixie) AMD64 | ✓ Supported | curl downloader only (DO not available) |

## Reference Agent

This repository provides a tested **reference implementation** of the Device Update agent. It demonstrates how devices can communicate with and receive update deployments from the Azure Device Update service.
 
Customers are responsible for building, validating, and adapting this code for use in their environments to meet their specific requirements.
 
We encourage you to extend and modify the implementation to fit your scenarios. If you have ideas, suggestions, or encounter issues, please use [GitHub Issues](https://github.com/Azure/iot-hub-device-update/issues) or the [Discussion Forum](https://github.com/Azure/iot-hub-device-update/discussions).

## Obtaining the Agent

ADU no longer publishes new versions of `deviceupdate-agent` to [packages.microsoft.com](https://packages.microsoft.com) (PMC). Previously published packages may still be available there, but they do not track current GitHub releases.

To use a current release, select its release tag from [GitHub Releases](https://github.com/Azure/iot-hub-device-update/releases), check out that tag, and follow the [build instructions](./docs/agent-reference/how-to-build-agent-code.md) that accompany the selected release. Build the agent from source for your target Linux distribution and architecture. Building the `develop` branch instead uses development code, not a selected release.

The [supported-platform list](#supported-platforms) describes platforms supported by the reference implementation; it does not indicate availability of current prebuilt agent packages on PMC.

This distribution notice does not change support or end-of-life policies and applies only to the reference agent, not other products or dependencies hosted on PMC.

## Getting Started

* [Device Update for IoT Hub](https://aka.ms/iot-hub-device-update-docs)
* [Getting Started with Device Update Agent](./docs/agent-reference)
* More details on building the agent here: [How to build agent code](./docs/agent-reference/how-to-build-agent-code.md)
* For remote diagnostics and log collection: [Diagnostics Log Collection](./docs/diagnostics-log-collection.md)

To build and install a local Debian package, first check out your selected release tag and consult its build instructions, then run:

```sh
./scripts/install-deps.sh -a
./scripts/build.sh -c -u --build-packages
sudo apt install ./out/deviceupdate-agent_*.deb
```

`sudo apt install ./out/deviceupdate-agent_*.deb` installs the local build output, not the agent from PMC.

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
