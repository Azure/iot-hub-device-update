# Install from Debian packages hosted on packages.microsoft.com

## Prerequisites

- A Raspberry Pi 3 or 4 with a supported OS image
- Network connectivity
- Access to [packages.microsoft.com](https://packages.microsoft.com)

> **Note**: Debian 10 (Buster) has reached end of life and is no longer supported.
> Ubuntu 18.04 has also reached end of life. Use Ubuntu 22.04+ or Debian 11+ for new deployments.

## Raspberry Pi 3

### Ubuntu Server 22.04 LTS (64-bit)

1. From RPi Imager, choose `Other general-purpose OS -> Ubuntu Server 22.04 LTS (64-bit)`
2. [Setup packages.microsoft.com apt repository](https://learn.microsoft.com/en-us/windows-server/administration/linux-package-repository-for-microsoft-software#ubuntu)
3. Install the agent:

```bash
sudo apt update
sudo apt install deviceupdate-agent
```

### Debian 11 (Bullseye) armhf

1. From RPi Imager, choose `Raspberry Pi OS -> Raspberry Pi OS Lite`
2. [Setup packages.microsoft.com apt repository for Debian 11](https://learn.microsoft.com/en-us/windows-server/administration/linux-package-repository-for-microsoft-software#debian)
3. Install the agent:

```bash
sudo apt update
sudo apt install deviceupdate-agent
```

## Raspberry Pi 4

### Ubuntu 22.04 LTS (aarch64)

1. From RPi Imager, choose `Other general-purpose OS -> Ubuntu Server 22.04 LTS (64-bit)`
2. [Setup packages.microsoft.com apt repository](https://learn.microsoft.com/en-us/windows-server/administration/linux-package-repository-for-microsoft-software#ubuntu)
3. Install the agent:

```bash
sudo apt update
sudo apt install deviceupdate-agent
```

### Ubuntu 24.04 LTS (aarch64)

1. From RPi Imager, choose `Other general-purpose OS -> Ubuntu Server 24.04 LTS (64-bit)`
2. [Setup packages.microsoft.com apt repository](https://learn.microsoft.com/en-us/windows-server/administration/linux-package-repository-for-microsoft-software#ubuntu)
3. Install the agent:

```bash
sudo apt update
sudo apt install deviceupdate-agent
```

> **Note**: On Ubuntu 24.04, the agent uses the curl content downloader by default
> since Delivery Optimization packages are not yet available for this release.

### Debian 12 (Bookworm) arm64

1. Flash Debian 12 arm64 image for Raspberry Pi 4
2. [Setup packages.microsoft.com apt repository for Debian 12](https://learn.microsoft.com/en-us/windows-server/administration/linux-package-repository-for-microsoft-software#debian)
3. Install the agent:

```bash
sudo apt update
sudo apt install deviceupdate-agent
```

## Building from Source

If pre-built packages are not available for your platform, you can build the agent from source.
See [Building the Agent](how-to-build-agent-code.md) for complete instructions.

## Next Steps

- [How to Run the Agent](how-to-run-agent.md)
- [Configuration Guide](configuration-guide.md)
- [Troubleshooting](../how-to-troubleshoot-guide.md)
