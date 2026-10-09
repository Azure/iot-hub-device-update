# Historical agent packages on PMC

ADU no longer publishes new versions of `deviceupdate-agent` to [packages.microsoft.com](https://packages.microsoft.com) (PMC). Previously published packages may still be available there, but they do not track current GitHub releases.

For current releases, select a release tag from [GitHub Releases](https://github.com/Azure/iot-hub-device-update/releases), check out that tag, and follow [Building the Agent](how-to-build-agent-code.md) as provided in that tag for your target Linux distribution and architecture. See [Obtaining the Agent](../../README.md#obtaining-the-agent) for the scope of this distribution policy.

## Prerequisites

- A Raspberry Pi 3 or 4 with an OS image compatible with the historical package you intend to install
- Network connectivity
- Access to [packages.microsoft.com](https://packages.microsoft.com)

> **Note**: Debian 10 (Buster) has reached end of life and is no longer supported.
> Ubuntu 18.04 has also reached end of life. For new deployments, follow the current-release source-build path above.

## Historical Installation Examples

The commands below are retained only for previously published packages, if available. Verify package availability and compatibility for your exact distribution and architecture before proceeding; these examples do not guarantee present or future availability on the listed platforms. Do not assume historical packages include features described for current GitHub releases.

### Raspberry Pi 3

#### Ubuntu Server 22.04 LTS (64-bit)

1. From RPi Imager, choose `Other general-purpose OS -> Ubuntu Server 22.04 LTS (64-bit)`
2. [Setup packages.microsoft.com apt repository](https://learn.microsoft.com/en-us/windows-server/administration/linux-package-repository-for-microsoft-software#ubuntu)
3. Install the historical agent package, if available:

```bash
sudo apt update
sudo apt install deviceupdate-agent
```

#### Debian 11 (Bullseye) armhf

1. From RPi Imager, choose `Raspberry Pi OS -> Raspberry Pi OS Lite`
2. [Setup packages.microsoft.com apt repository for Debian 11](https://learn.microsoft.com/en-us/windows-server/administration/linux-package-repository-for-microsoft-software#debian)
3. Install the historical agent package, if available:

```bash
sudo apt update
sudo apt install deviceupdate-agent
```

### Raspberry Pi 4

#### Ubuntu 22.04 LTS (aarch64)

1. From RPi Imager, choose `Other general-purpose OS -> Ubuntu Server 22.04 LTS (64-bit)`
2. [Setup packages.microsoft.com apt repository](https://learn.microsoft.com/en-us/windows-server/administration/linux-package-repository-for-microsoft-software#ubuntu)
3. Install the historical agent package, if available:

```bash
sudo apt update
sudo apt install deviceupdate-agent
```

#### Ubuntu 24.04 LTS (aarch64)

> **Source builds**: Consult the [platform compatibility matrix](how-to-build-agent-code.md#platform-compatibility-matrix) in your selected release's build guide for Ubuntu 24.04 platform and content-downloader requirements. This source-build guidance does not establish the defaults or features of historical PMC packages.

1. From RPi Imager, choose `Other general-purpose OS -> Ubuntu Server 24.04 LTS (64-bit)`
2. [Setup packages.microsoft.com apt repository](https://learn.microsoft.com/en-us/windows-server/administration/linux-package-repository-for-microsoft-software#ubuntu)
3. Install the historical agent package, if available:

```bash
sudo apt update
sudo apt install deviceupdate-agent
```

#### Debian 12 (Bookworm) arm64

1. Flash Debian 12 arm64 image for Raspberry Pi 4
2. [Setup packages.microsoft.com apt repository for Debian 12](https://learn.microsoft.com/en-us/windows-server/administration/linux-package-repository-for-microsoft-software#debian)
3. Install the historical agent package, if available:

```bash
sudo apt update
sudo apt install deviceupdate-agent
```

## Building from Source

Current releases require building the agent from source for your target Linux distribution and architecture, regardless of whether older PMC packages are available. Select a release tag from [GitHub Releases](https://github.com/Azure/iot-hub-device-update/releases) and follow [Building the Agent](how-to-build-agent-code.md) in the selected tag.

## Next Steps

- [How to Run the Agent](how-to-run-agent.md)
- [Configuration Guide](configuration-guide.md)
- [Troubleshooting](../how-to-troubleshoot-guide.md)
