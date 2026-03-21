# Agent Integration Guide

This guide covers how to integrate the Device Update agent into a Linux-based
device image. It focuses on the recommended systemd approach, then describes
alternatives for environments without systemd (Yocto/BusyBox init, OpenRC,
container, or bare-process).

For a quick "build-and-run in 15 minutes" walkthrough, see
[Quick Start](quick-start.md).

---

## User and Group Setup

The agent must run as a dedicated unprivileged user. All supported integration
methods require this step.

```bash
# Create the system group and user
sudo addgroup --system adu
sudo adduser --system adu --ingroup adu --no-create-home --shell /bin/false

# Allow writing to syslog
sudo usermod -aG syslog adu

# If using Delivery Optimization
sudo usermod -aG do adu           # adu can access DO resources
sudo usermod -aG adu do           # DO can write to the ADU sandbox
sudo systemctl restart deliveryoptimization-agent   # pick up group change

# If using Azure IoT Identity Service (AIS)
sudo usermod -aG aziotid adu
sudo usermod -aG aziotcs adu
sudo usermod -aG aziotks adu
```

> The Debian package (`postinst`) and the CMake install script
> (`daemon/install.sh`) perform these steps automatically. You only need to
> run them manually for custom or development installs.

### adu-shell Permissions

adu-shell is the setuid-root helper that performs privileged operations on
behalf of the agent. It must be owned by `root:adu` with the setuid bit set.

```bash
sudo chown root:adu /usr/bin/adu-shell
sudo chmod u=rxs,g=rx,o= /usr/bin/adu-shell    # octal 4550
```

For a full explanation of the privilege model, see the
[adu-shell README](../../src/adu-shell/README.md).

---

## Recommended: systemd Service

Most Linux distributions (Ubuntu, Debian, Raspberry Pi OS, Yocto with systemd)
use systemd as the init system. The agent ships with a ready-made unit file.

### Unit File

Installed to `/lib/systemd/system/deviceupdate-agent.service`:

```ini
[Unit]
Description=Device Update Agent daemon.
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
Restart=always
RestartSec=5
User=adu
Group=adu
ExecStart=/usr/bin/AducIotAgent -l 0 -e

[Install]
WantedBy=multi-user.target
```

Key points:

- **`User=adu, Group=adu`** — the agent never runs as root.
- **`Restart=always`** — systemd restarts the agent on crash.
- **`After=network-online.target`** — waits for network connectivity before
  starting.

### Managing the Service

```bash
sudo systemctl enable deviceupdate-agent    # start on boot
sudo systemctl start  deviceupdate-agent    # start now
sudo systemctl status deviceupdate-agent    # check status
sudo systemctl stop   deviceupdate-agent    # stop
journalctl -fu deviceupdate-agent           # tail logs
```

---

## Alternative: SysVinit / init.d

For distributions that use SysVinit (older Debian, some Yocto images), create
an init script that starts the agent as the `adu` user.

```bash
#!/bin/sh
### BEGIN INIT INFO
# Provides:          deviceupdate-agent
# Required-Start:    $network $remote_fs
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Description:       Azure Device Update Agent
### END INIT INFO

DAEMON=/usr/bin/AducIotAgent
DAEMON_ARGS="-l 0 -e"
RUNAS=adu

case "$1" in
  start)
    echo "Starting deviceupdate-agent"
    start-stop-daemon --start --background --chuid "$RUNAS" --exec "$DAEMON" -- $DAEMON_ARGS
    ;;
  stop)
    echo "Stopping deviceupdate-agent"
    start-stop-daemon --stop --exec "$DAEMON"
    ;;
  *)
    echo "Usage: $0 {start|stop}"
    exit 1
    ;;
esac
```

Install with:

```bash
sudo cp deviceupdate-agent /etc/init.d/
sudo chmod +x /etc/init.d/deviceupdate-agent
sudo update-rc.d deviceupdate-agent defaults
```

---

## Alternative: BusyBox init / OpenRC

For minimal images (BusyBox-based, Alpine with OpenRC), the agent can be
started from `/etc/inittab` or an OpenRC service script. The key requirements
are:

1. The process must run as user `adu` (use `su -s /bin/sh adu -c ...` or
   `start-stop-daemon --chuid adu`).
2. Network must be available before the agent starts.
3. The process should be restarted on crash (use `respawn` in inittab, or
   `supervise-daemon` in OpenRC).

---

## Alternative: Container Deployment

When running the agent inside a Docker or OCI container, the same user/group
and adu-shell permission setup applies. See the Docker templates in
`scripts/docker/` for a working example.

Considerations for containers:

- The container must have access to the host network (or be configured for the
  target IoT Hub endpoint).
- adu-shell still requires the setuid bit — the container must not drop
  `CAP_SETUID` from its security context.
- The `du-config.json` file is typically mounted from the host.

---

## CLI Reference

The `AducIotAgent` binary accepts the following command-line options.

| Option | Argument | Description |
|--------|----------|-------------|
| `--version` | — | Print the agent version and exit |
| `--health-check` | — | Run startup health checks (connection info, file permissions) and exit |
| `--enable-iothub-tracing` | — | Enable verbose tracing from the Azure IoT C SDK (useful for connection troubleshooting) |
| `--log-level` | `0`–`3` | Set log verbosity: `0` = Debug, `1` = Info, `2` = Warning, `3` = Error |
| `--deviceinfo_manufacturer=` | string | Override the manufacturer reported via the DeviceInformation PnP interface |
| `--deviceinfo_model=` | string | Override the model reported via the DeviceInformation PnP interface |
| `--deviceinfo_swversion=` | string | Override the software version reported via the DeviceInformation PnP interface |

Example:

```bash
sudo -u adu /usr/bin/AducIotAgent \
  --enable-iothub-tracing \
  --log-level 0
```

> **Note:** The connection string is no longer passed on the command line.
> Configure it in `du-config.json` instead. See the
> [Configuration Guide](configuration-guide.md).

---

## Related Documentation

- [Quick Start](quick-start.md) — build and run in 15 minutes
- [Configuration Guide](configuration-guide.md) — `du-config.json` reference
- [adu-shell README](../../src/adu-shell/README.md) — privilege model and
  process boundary diagram
- [How to Build Agent Code](how-to-build-agent-code.md) — build instructions
- [Troubleshooting](../how-to-troubleshoot-guide.md) — common issues and
  diagnostics
