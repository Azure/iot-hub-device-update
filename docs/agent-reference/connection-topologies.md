# Connection Topologies

This document covers all supported methods for connecting a leaf device running the Azure Device Update (ADU) agent to Azure IoT Hub. It describes the available authentication mechanisms, network topologies, and their configurations in `du-config.json`.

For general agent configuration, see [configuration-guide.md](configuration-guide.md). For X.509 certificate setup details, see [how-to-x509-authentication.md](how-to-x509-authentication.md).

---

## Connection Methods

The ADU agent supports three connection methods, configured via the `connectionType` field inside each agent's `connectionSource` block in `/etc/adu/du-config.json`.

### SAS Token (Connection String)

| Field | Value |
|---|---|
| `connectionType` | `"string"` |
| `connectionData` | Full IoT Hub connection string with `SharedAccessKey` |

This is the simplest method. You copy the device connection string from the Azure Portal (**IoT Hub → Devices → \<device\> → Primary Connection String**) and paste it directly into the config file.

**Connection string format:**

```
HostName=<hub-name>.azure-devices.net;DeviceId=<device-id>;SharedAccessKey=<base64-key>
```

Best suited for development and testing. In production, prefer X.509 or AIS for stronger security guarantees.

### Azure Identity Service (AIS)

| Field | Value |
|---|---|
| `connectionType` | `"AIS"` |
| `connectionData` | AIS principal name (typically `"iotHubDeviceUpdate"`) |

When the device runs IoT Edge (or uses the `aziot-identity-service` standalone), the ADU agent can delegate authentication entirely to the **Azure IoT Identity Service (AIS/EIS)**. The agent does not hold any secrets itself — it requests a connection token from AIS at runtime.

AIS supports both **SAS** and **X.509** auth types, depending on how the IoT Edge device was provisioned. The ADU agent does not need to know which auth type is in use; AIS handles it transparently.

**Prerequisites:**

- The `aziot-identity-service` package is installed and configured.
- The `adu` user is a member of the `aziotid`, `aziotcs`, and `aziotks` groups.
- The AIS principal `iotHubDeviceUpdate` is registered in the IoT Identity Service configuration.

#### AIS Module Identity Registration (adu.toml)

When the ADU agent Debian package is installed, the post-install script automatically registers a **module identity principal** with the Azure IoT Identity Service by creating `/etc/aziot/identityd/config.d/adu.toml`:

```toml
[[principal]]
 name="IoTHubDeviceUpdate"
 idtype=["module"]
 uid= <adu-user-uid>
```

**What this does:**

The AIS identity service uses UID-based access control. When the ADU agent process (running as `adu` user) makes a request to the identity service Unix Domain Socket (`/run/aziot/identityd.sock`), AIS looks up the calling process UID and returns the identity mapped to that principal.

The `idtype=["module"]` declaration is critical — it tells AIS to provision a **Module identity** (e.g., `DeviceId=mydevice;ModuleId=IoTHubDeviceUpdate`) rather than returning the bare device identity.

**Why Module Identity Matters:**

| Scenario | Identity Type | Twin Used | Risk |
|----------|--------------|-----------|------|
| Without `adu.toml` | Device identity | Device Twin | ⚠️ Connection contention with other apps |
| With `adu.toml` (default) | Module identity | Module Twin | ✅ Isolated, no contention |

Without the module identity registration:
- AIS may return a **Device connection string** to the ADU agent
- The ADU agent would communicate via the **Device Twin**
- Other applications on the same device (e.g., IoT Edge modules, custom apps) that also use the Device Twin will experience **connection contention** — only one active MQTT connection per identity is allowed by IoT Hub
- This causes intermittent disconnections and missed twin updates

With the module identity registration (default):
- AIS returns a **Module connection string** (`ModuleId=IoTHubDeviceUpdate`)
- The ADU agent communicates via its own dedicated **Module Twin**
- The Device Twin remains available for other applications
- Each module gets its own independent MQTT connection — no contention

**Manual Setup (if not using the Debian package):**

If you build and install the ADU agent from source, you must manually create the TOML registration:

```bash
# Create the TOML registration for AIS
sudo bash -c 'printf "[[principal]]\n name=\"IoTHubDeviceUpdate\"\n idtype=[\"module\"]\n uid= $(id -u adu)\n" > /etc/aziot/identityd/config.d/adu.toml'

# Set correct ownership and permissions
sudo chown aziotid:aziotid /etc/aziot/identityd/config.d/adu.toml
sudo chmod u=rw /etc/aziot/identityd/config.d/adu.toml

# Restart the identity service to pick up the new principal
sudo systemctl restart aziot-identityd
```

**Verifying the Registration:**

After registration, the ADU agent will receive a module identity when it queries AIS. You can verify by checking the agent logs:

```
Info: Attempting to get connection info from Identity Service (EIS)
Info: Attempting to create connection to IotHub using type: Module
```

If you see `type: Device` instead of `type: Module`, the TOML registration is missing or misconfigured.

> **Note:** The `connectionData` field value (`"iotHubDeviceUpdate"`) in `du-config.json` corresponds to the principal `name` in the TOML file. While the current agent code does not pass this value to AIS directly (AIS uses UID-based lookup), it serves as documentation and may be used in future versions for multi-principal scenarios.

### X.509 Certificate

| Field | Value |
|---|---|
| `connectionType` | `"X509"` |
| `connectionData` | Connection string with `x509=true` (no SharedAccessKey) |
| `connectionX509CertFilePath` | Path to client certificate (PEM) |
| `connectionX509PrivateKeyFilePath` | Path to private key (PEM or PKCS#11 URI) |
| `connectionX509CaCertFilePath` | Path to CA certificate chain (PEM) |

The agent authenticates directly to IoT Hub using mutual TLS (mTLS). The device presents a client certificate during the TLS handshake, and IoT Hub validates it against the registered thumbprint or enrollment group CA.

> **Requires `schemaVersion` 1.2 or later in `du-config.json`.**

**Connection string format:**

```
HostName=<hub-name>.azure-devices.net;DeviceId=<device-id>;x509=true
```

For module identity (IoT Edge module):

```
HostName=<hub-name>.azure-devices.net;DeviceId=<device-id>;ModuleId=<module-id>;x509=true
```

**Certificate file permissions:**

```bash
sudo chmod 644 /etc/adu/certs/client.pem
sudo chmod 644 /etc/adu/certs/ca.pem
sudo chmod 600 /etc/adu/certs/client.key
sudo chown -R adu:adu /etc/adu/certs/
```

For PKCS#11 / HSM-backed keys, set the `opensslEngine` field to `"pkcs11"` and use a PKCS#11 URI for the private key path. See [how-to-x509-authentication.md](how-to-x509-authentication.md) for details.

---

## Network Topologies

### Direct to IoT Hub

```
┌──────────┐       internet       ┌─────────────────────┐
│  Device   │ ──────────────────▶ │  Azure IoT Hub      │
│ (ADU Agent)│   MQTT/8883 or     │  *.azure-devices.net │
└──────────┘   MQTT-WS/443       └─────────────────────┘
```

The device has direct internet connectivity and connects to `<hub>.azure-devices.net` over MQTT (port 8883) or MQTT over WebSockets (port 443).

This is the default topology. No special configuration is needed beyond setting the connection method (SAS, X.509, or AIS).

### Through Edge Gateway (Nested Edge)

```
┌──────────┐    local network    ┌────────────────┐    internet    ┌──────────────┐
│  Leaf     │ ─────────────────▶ │  IoT Edge      │ ────────────▶ │  Azure       │
│  Device   │   MQTT/8883        │  Gateway        │               │  IoT Hub     │
└──────────┘                     └────────────────┘               └──────────────┘
```

The leaf device connects to a **parent IoT Edge device** rather than directly to IoT Hub. The Edge gateway relays all IoT Hub traffic upstream. This is used when:

- Devices are on an isolated network segment.
- You need a single egress point for fleet traffic.
- You are deploying a nested IoT Edge hierarchy.

**Configuration requirements:**

1. **`GatewayHostName`** — Append `GatewayHostName=<gateway-hostname>` to the device connection string so the MQTT client connects to the gateway instead of `*.azure-devices.net`.

2. **`edgegatewayCertPath`** — Set the top-level field in `du-config.json` to the path of the gateway's root CA certificate. The leaf device needs this certificate to trust the TLS connection to the gateway (the gateway typically uses a self-signed or private CA).

**Connection string format (SAS with gateway):**

```
HostName=<hub>.azure-devices.net;DeviceId=<device-id>;SharedAccessKey=<key>;GatewayHostName=<gateway-hostname>
```

**Connection string format (X.509 with gateway):**

```
HostName=<hub>.azure-devices.net;DeviceId=<device-id>;x509=true;GatewayHostName=<gateway-hostname>
```

### Offline Leaf Device

```
┌──────────┐    local only    ┌────────────────────────┐    internet    ┌──────────┐
│  Leaf     │ ──────────────▶ │  IoT Edge Gateway      │ ────────────▶ │  Azure   │
│  Device   │  no internet    │  + Microsoft Connected  │               │  IoT Hub │
│           │                 │    Cache (MCC)          │               │          │
└──────────┘                  └────────────────────────┘               └──────────┘
```

A stricter variant of the gateway topology where the leaf device has **no internet access at all** — not even DNS resolution for public endpoints. All traffic, including update content downloads, must flow through the parent gateway.

**Requirements:**

- The parent IoT Edge gateway must run **Microsoft Connected Cache (MCC)** so that update payloads can be cached and served to the leaf device over the local network.
- The leaf device connection string must include `GatewayHostName`.
- `edgegatewayCertPath` must be configured in `du-config.json`.
- The Delivery Optimization (DO) client on the leaf device must be configured to use the gateway as its MCC source.

---

## Configuration Matrix

The table below shows all valid combinations of authentication method and network topology, along with the key fields required in `du-config.json`.

| Auth Method | Topology | `connectionType` | `connectionData` format | `edgegatewayCertPath` | X.509 cert fields |
|---|---|---|---|---|---|
| SAS | Direct | `"string"` | `HostName=...;DeviceId=...;SharedAccessKey=...` | — | — |
| SAS | Gateway | `"string"` | `HostName=...;DeviceId=...;SharedAccessKey=...;GatewayHostName=...` | Required | — |
| X.509 | Direct | `"X509"` | `HostName=...;DeviceId=...;x509=true` | — | Required |
| X.509 | Gateway | `"X509"` | `HostName=...;DeviceId=...;x509=true;GatewayHostName=...` | Required | Required |
| AIS (SAS) | Direct | `"AIS"` | `"iotHubDeviceUpdate"` | — | — |
| AIS (SAS) | Gateway | `"AIS"` | `"iotHubDeviceUpdate"` | Required | — |
| AIS (X.509) | Direct | `"AIS"` | `"iotHubDeviceUpdate"` | — | — |
| AIS (X.509) | Gateway | `"AIS"` | `"iotHubDeviceUpdate"` | Required | — |

> **Note:** When using AIS, the underlying auth type (SAS or X.509) is configured in the Azure IoT Identity Service, not in `du-config.json`. The ADU agent config looks identical for both AIS-SAS and AIS-X.509.

---

## du-config.json Examples

### Direct SAS

```json
{
  "schemaVersion": "1.2",
  "aduShellTrustedUsers": ["adu", "do"],
  "manufacturer": "Contoso",
  "model": "Smart-Vacuum-v1",
  "agents": [
    {
      "name": "main",
      "runas": "adu",
      "connectionSource": {
        "connectionType": "string",
        "connectionData": "HostName=contoso-hub.azure-devices.net;DeviceId=vacuum-001;SharedAccessKey=abc123..."
      },
      "manufacturer": "Contoso",
      "model": "Smart-Vacuum-v1"
    }
  ]
}
```

### Direct X.509

```json
{
  "schemaVersion": "1.2",
  "aduShellTrustedUsers": ["adu", "do"],
  "manufacturer": "Contoso",
  "model": "Smart-Vacuum-v1",
  "agents": [
    {
      "name": "main",
      "runas": "adu",
      "connectionSource": {
        "connectionType": "X509",
        "connectionData": "HostName=contoso-hub.azure-devices.net;DeviceId=vacuum-001;x509=true",
        "connectionX509CertFilePath": "/etc/adu/certs/client.pem",
        "connectionX509PrivateKeyFilePath": "/etc/adu/certs/client.key",
        "connectionX509CaCertFilePath": "/etc/adu/certs/ca.pem"
      },
      "manufacturer": "Contoso",
      "model": "Smart-Vacuum-v1"
    }
  ]
}
```

### AIS (IoT Edge Module)

```json
{
  "schemaVersion": "1.2",
  "aduShellTrustedUsers": ["adu", "do"],
  "manufacturer": "Contoso",
  "model": "Edge-Gateway-v1",
  "agents": [
    {
      "name": "main",
      "runas": "adu",
      "connectionSource": {
        "connectionType": "AIS",
        "connectionData": "iotHubDeviceUpdate"
      },
      "manufacturer": "Contoso",
      "model": "Edge-Gateway-v1"
    }
  ]
}
```

### Nested Edge with SAS Token

```json
{
  "schemaVersion": "1.2",
  "aduShellTrustedUsers": ["adu", "do"],
  "edgegatewayCertPath": "/etc/adu/certs/edge-gateway-ca.pem",
  "manufacturer": "Contoso",
  "model": "Leaf-Sensor-v1",
  "agents": [
    {
      "name": "main",
      "runas": "adu",
      "connectionSource": {
        "connectionType": "string",
        "connectionData": "HostName=contoso-hub.azure-devices.net;DeviceId=leaf-sensor-001;SharedAccessKey=abc123...;GatewayHostName=edge-gw-01.local"
      },
      "manufacturer": "Contoso",
      "model": "Leaf-Sensor-v1"
    }
  ]
}
```

### Nested Edge with X.509

```json
{
  "schemaVersion": "1.2",
  "aduShellTrustedUsers": ["adu", "do"],
  "edgegatewayCertPath": "/etc/adu/certs/edge-gateway-ca.pem",
  "manufacturer": "Contoso",
  "model": "Leaf-Sensor-v1",
  "agents": [
    {
      "name": "main",
      "runas": "adu",
      "connectionSource": {
        "connectionType": "X509",
        "connectionData": "HostName=contoso-hub.azure-devices.net;DeviceId=leaf-sensor-001;x509=true;GatewayHostName=edge-gw-01.local",
        "connectionX509CertFilePath": "/etc/adu/certs/client.pem",
        "connectionX509PrivateKeyFilePath": "/etc/adu/certs/client.key",
        "connectionX509CaCertFilePath": "/etc/adu/certs/ca.pem"
      },
      "manufacturer": "Contoso",
      "model": "Leaf-Sensor-v1"
    }
  ]
}
```

---

## Certificate Roles

Two distinct categories of certificate appear in ADU agent configuration. Understanding their different purposes is essential when troubleshooting TLS failures.

### Client Identity Certificate (mTLS)

**Fields:** `connectionX509CertFilePath`, `connectionX509PrivateKeyFilePath`, `connectionX509CaCertFilePath`

These certificates prove the **device's identity** to IoT Hub during the TLS handshake. IoT Hub (or the Edge gateway acting on its behalf) validates the client certificate against the device's registered thumbprint or the enrollment group's CA chain.

- Used **only** when `connectionType` is `"X509"`.
- The private key must be readable by the `adu` user (mode `0600`).
- The CA cert (`connectionX509CaCertFilePath`) is the CA chain that issued the client cert — IoT Hub uses it to build the trust chain.

### Trust Anchor Certificate (Server Validation)

**Field:** `edgegatewayCertPath` (top-level in `du-config.json`)

This certificate allows the device to **trust the TLS server** it connects to. When the device connects directly to IoT Hub, no trust anchor is needed — IoT Hub uses a publicly trusted CA (DigiCert / Microsoft). When the device connects through an **Edge gateway**, the gateway presents a TLS certificate signed by a private or self-signed CA. The device must have that CA certificate to validate the TLS connection.

- Used in **any gateway topology** regardless of the auth method (SAS, X.509, or AIS).
- Points to the root CA certificate that signed the Edge gateway's server certificate.
- If missing or incorrect, the TLS handshake fails before authentication even begins.

**Summary:**

| Certificate | Purpose | When needed | Configured in |
|---|---|---|---|
| Client cert + key | Proves device identity (mTLS) | `connectionType: "X509"` | `connectionSource` block |
| Gateway trust anchor | Validates gateway TLS cert | Any gateway topology | Top-level `edgegatewayCertPath` |

---

## Troubleshooting

### X.509 works direct but fails through Edge gateway

**Symptom:** The agent connects successfully with `connectionType: "X509"` when pointed at IoT Hub directly, but fails with a TLS error after adding `GatewayHostName` to the connection string.

**Cause:** The `edgegatewayCertPath` field is missing or points to the wrong certificate. When connecting through a gateway, the device needs two independent certificate chains:

1. The **client identity** chain (configured in `connectionSource`) to prove who the device is.
2. The **server trust** chain (configured in `edgegatewayCertPath`) to verify the gateway's TLS certificate.

Without the trust anchor, the TLS handshake fails before the client certificate is ever presented.

**Fix:** Set `edgegatewayCertPath` to the root CA that signed the Edge gateway's TLS server certificate. This is typically the IoT Edge device CA — not the same CA that signed the client identity cert.

### TLS handshake failures

**Symptom:** Agent logs show `TLS handshake failed`, `SSL_ERROR_SYSCALL`, or `CERTIFICATE_VERIFY_FAILED`.

**Common causes:**

| Cause | Check |
|---|---|
| Expired certificate | `openssl x509 -in <cert> -noout -dates` |
| Wrong CA chain | Verify `edgegatewayCertPath` matches the gateway's issuing CA |
| Hostname mismatch | The `GatewayHostName` must match the CN or SAN in the gateway's TLS cert |
| Clock skew | Ensure device time is synchronized (certificates are time-sensitive) |
| Missing intermediate CAs | The CA file must contain the full chain from the signing CA to the root |

### Certificate path and permission issues

**Symptom:** Agent fails to start or logs `unable to read certificate` / `permission denied`.

**Checklist:**

```bash
# Verify files exist and are readable by the adu user
sudo -u adu cat /etc/adu/certs/client.pem > /dev/null && echo "OK" || echo "FAIL"
sudo -u adu cat /etc/adu/certs/client.key > /dev/null && echo "OK" || echo "FAIL"

# Verify PEM format
openssl x509 -in /etc/adu/certs/client.pem -noout -text | head -5
openssl rsa  -in /etc/adu/certs/client.key -noout -check

# Verify cert and key match
openssl x509 -in /etc/adu/certs/client.pem -noout -modulus | md5sum
openssl rsa  -in /etc/adu/certs/client.key -noout -modulus | md5sum
# Both md5sums must be identical
```

### AIS connection failures

**Symptom:** Agent logs `Failed to get device identity from Identity Service` or `aziot-identityd` errors.

**Checklist:**

- Verify the `adu` user is in the required groups: `id adu` should show `aziotid`, `aziotcs`, `aziotks`.
- Verify the identity service is running: `sudo systemctl status aziot-identityd`.
- Verify the principal is configured: check `/etc/aziot/config.toml` for a `[[principal]]` entry with `name = "iotHubDeviceUpdate"` and `uid` matching the `adu` user.

---

## See Also

- [configuration-guide.md](configuration-guide.md) — Full `du-config.json` field reference
- [how-to-x509-authentication.md](how-to-x509-authentication.md) — Detailed X.509 setup, PKCS#11, and certificate generation
- [how-to-run-agent.md](how-to-run-agent.md) — Running the agent and daemon setup
- [architecture-overview.md](architecture-overview.md) — Agent architecture and component overview
