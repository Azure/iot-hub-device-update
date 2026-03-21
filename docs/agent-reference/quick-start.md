# Quick Start: Azure Device Update Agent v1.3.0-rc1

Get the Device Update agent built and running on a Linux device in ~15 minutes.

## Prerequisites

| Requirement | Details |
|-------------|---------|
| **OS** | Ubuntu 20.04 / 22.04 / 24.04, Debian 11 / 12 / 13 (amd64, arm64, arm32) |
| **Tools** | git, cmake 3.5+ (the install script provides 3.23.2) |
| **IoT Hub** | An Azure IoT Hub with a registered device and its connection string |

## 1. Clone & Install Dependencies

```bash
git clone https://github.com/Azure/iot-hub-device-update.git
cd iot-hub-device-update

# Install all dependencies (packages, Azure IoT SDK, Delivery Optimization, cmake)
sudo ./scripts/install-deps.sh -a
```

> **Tip:** Use `--install-packages-only` for a minimal install if you already have
> the Azure IoT C SDK and Delivery Optimization built. See
> [how-to-build-agent-code.md](how-to-build-agent-code.md) for all options.

## 2. Build the Agent

```bash
# Clean build in Debug mode (default)
./scripts/build.sh -c
```

Binaries are written to `out/bin/`. Common build options:

| Flag | Purpose |
|------|---------|
| `-c` | Clean build (removes previous output) |
| `-t Release` | Release build instead of Debug |
| `-u` | Include unit tests |
| `--build-packages` | Generate `.deb` packages |

See [how-to-build-agent-code.md](how-to-build-agent-code.md) for the full list.

## 3. Configure

Create `/etc/adu/du-config.json` with your IoT Hub connection string:

```bash
sudo mkdir -p /etc/adu
sudo nano /etc/adu/du-config.json
```

Minimal configuration:

```json
{
  "schemaVersion": "1.2",
  "aduShellTrustedUsers": ["adu"],
  "manufacturer": "contoso",
  "model": "my-device",
  "agents": [
    {
      "name": "main",
      "runas": "adu",
      "connectionSource": {
        "connectionType": "string",
        "connectionData": "HostName=<your-hub>.azure-devices.net;DeviceId=<device-id>;SharedAccessKey=<key>"
      },
      "manufacturer": "contoso",
      "model": "my-device"
    }
  ]
}
```

See [configuration-guide.md](configuration-guide.md) for the full configuration reference.

## 4. Set Up the Runtime User

The agent runs as the `adu` user. Create it and set up `adu-shell`:

```bash
# Create the adu group and user
sudo addgroup --system adu
sudo adduser --system adu --ingroup adu --no-create-home --shell /bin/false
sudo usermod -aG syslog adu

# Install adu-shell (required for privileged operations)
sudo mkdir -p /usr/lib/adu
sudo cp out/bin/adu-shell /usr/lib/adu/
sudo chown root:adu /usr/lib/adu/adu-shell
sudo chmod u=rxs /usr/lib/adu/adu-shell

# Ensure log directory exists
sudo mkdir -p /var/log/adu
sudo chown adu:adu /var/log/adu
```

## 5. Run the Agent

```bash
# Run directly (foreground)
sudo -u adu out/bin/AducIotAgent --log-level 0

# Check logs
tail -f /var/log/adu/*.log
```

The agent will connect to IoT Hub and report its device properties. Verify the
connection in the Azure portal under your IoT Hub → Device Update.

## Next Steps

- [configuration-guide.md](configuration-guide.md) — Full configuration reference
- [how-to-x509-authentication.md](how-to-x509-authentication.md) — Certificate-based authentication
- [building-with-delta-handler.md](building-with-delta-handler.md) — Delta download support
- [how-to-build-agent-code.md](how-to-build-agent-code.md) — Advanced build options
- [Agent Integration Guide](agent-integration-guide.md) — systemd, init.d, container deployment, and CLI reference
