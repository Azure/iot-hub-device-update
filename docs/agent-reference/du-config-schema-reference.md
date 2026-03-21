# du-config.json Schema Reference

This document describes the `du-config.json` configuration schema across all
versions. Use it to understand which properties are available at each version,
what changed between versions, and how to migrate.

For field-by-field usage guidance, connection-type examples, and complete
sample configurations, see the [Configuration Guide](configuration-guide.md).

## Schema Versions at a Glance

| Schema Version | Agent Release | Status |
|----------------|--------------|--------|
| `"1.0"` | 1.0.0 (Nov 2022) | Legacy — functional but missing newer fields |
| `"1.1"` | 1.1.0 | Supported |
| `"1.2"` | 1.2.0+ | **Current — recommended for all new deployments** |

The agent reads `schemaVersion` at startup but does not reject unknown values.
All version-specific fields are parsed as optional regardless of the declared
version, so a v1.0 config will still load on a newer agent — you simply will
not have access to features introduced in later versions.

---

## Top-Level Properties

| Property | Type | Required | v1.0 | v1.1 | v1.2 | Default | Description |
|----------|------|----------|------|------|------|---------|-------------|
| `schemaVersion` | string | ✅ | ✅ | ✅ | ✅ | — | Schema version identifier |
| `manufacturer` | string | ✅ | ✅ | ✅ | ✅ | — | Device-info manufacturer |
| `model` | string | ✅ | ✅ | ✅ | ✅ | — | Device-info model |
| `aduShellTrustedUsers` | string[] | ✅ | ✅ | ✅ | ✅ | — | OS users allowed to invoke ADU shell |
| `agents` | object[] | ✅ | ✅ | ✅ | ✅ | — | Array of agent definitions (see below) |
| `edgegatewayCertPath` | string | ❌ | ✅ | ✅ | ✅ | — | IoT Edge gateway certificate path |
| `compatPropertyNames` | string | ❌ | ✅ | ✅ | ✅ | — | Comma-separated compat property names |
| `iotHubProtocol` | string | ❌ | — | ✅ | ✅ | `"mqtt"` | `"mqtt"` or `"mqtt/ws"` |
| `downloadTimeoutInMinutes` | unsigned int | ❌ | — | ✅ | ✅ | `0` (no timeout) | Payload download timeout |
| `aduShellFolder` | string | ❌ | — | ✅ | ✅ | build default | Directory containing ADU shell binary |
| `dataFolder` | string | ❌ | — | ✅ | ✅ | `/var/lib/adu` | Agent data directory |
| `downloadsFolder` | string | ❌ | — | ✅ | ✅ | `<dataFolder>/downloads` | Downloaded payload storage |
| `extensionsFolder` | string | ❌ | — | ✅ | ✅ | `<dataFolder>/extensions` | Extensions storage |
| `apiRequestFifoPath` | string | ❌ | — | — | ✅ | — | Cross-process API request FIFO path |
| `idlePauseMilliseconds` | unsigned int | ❌ | — | — | ✅ | `0` (disabled) | Quiet-period duration after an update completes |
| ~~`simulateUnhealthyState`~~ | ~~bool~~ | — | ⚠️ | ❌ | ❌ | — | *Removed — see Deprecated Fields* |

**Legend:** ✅ = supported, — = not available, ⚠️ = deprecated, ❌ = removed

---

## Agent Object Properties (`agents[]`)

Each entry in the `agents` array has the following shape:

| Property | Type | Required | v1.0 | v1.1 | v1.2 | Description |
|----------|------|----------|------|------|------|-------------|
| `name` | string | ✅ | ✅ | ✅ | ✅ | Agent identifier (e.g. `"main"`) |
| `runas` | string | ✅ | ✅ | ✅ | ✅ | OS user the agent runs as |
| `connectionSource` | object | ✅ | ✅ | ✅ | ✅ | Connection method (see below) |
| `manufacturer` | string | ✅ | ✅ | ✅ | ✅ | Device-properties manufacturer |
| `model` | string | ✅ | ✅ | ✅ | ✅ | Device-properties model |
| `additionalDeviceProperties` | object | ❌ | ✅ | ✅ | ✅ | Custom key-value pairs reported with device properties |

---

## Connection Source Properties (`agents[].connectionSource`)

| Property | Type | Required | v1.0 | v1.1 | v1.2 | Description |
|----------|------|----------|------|------|------|-------------|
| `connectionType` | string | ✅ | ✅ | ✅ | ✅ | `"string"`, `"AIS"`, or `"X509"` (v1.2+) |
| `connectionData` | string | ✅ | ✅ | ✅ | ✅ | Connection string or AIS principal name |
| `connectionX509CertFilePath` | string | ❌ | — | ✅ | ✅ | Path to X.509 client certificate PEM |
| `connectionX509PrivateKeyFilePath` | string | ❌ | — | ✅ | ✅ | Path to private key (PEM or PKCS#11 URI) |
| `connectionX509CaCertFilePath` | string | ❌ | — | ✅ | ✅ | Path to CA certificate PEM |
| `opensslEngine` | string | ❌ | — | ✅ | ✅ | OpenSSL engine for HSM-backed keys (e.g. `"pkcs11"`) |

> **Note:** All three X.509 file-path fields must be provided together. See
> the [Configuration Guide](configuration-guide.md#connection-types) for
> connection-type usage details and examples.

---

## What Changed Between Versions

### v1.0 → v1.1

| Change | Details |
|--------|---------|
| **Added** `iotHubProtocol` | Selects MQTT or MQTT-over-WebSockets transport |
| **Added** `downloadTimeoutInMinutes` | Configurable payload download timeout |
| **Added** path-override fields | `aduShellFolder`, `dataFolder`, `downloadsFolder`, `extensionsFolder` |
| **Added** X.509 certificate fields | `connectionX509CertFilePath`, `connectionX509PrivateKeyFilePath`, `connectionX509CaCertFilePath`, `opensslEngine` inside `connectionSource` |
| **Removed** `simulateUnhealthyState` | Simulator platform layer was removed |

### v1.1 → v1.2

| Change | Details |
|--------|---------|
| **Added** `connectionType: "X509"` | Dedicated X.509 connection type value (previously cert fields could only be used alongside `"AIS"` or `"string"` connection types) |
| **Added** `apiRequestFifoPath` | Named pipe for cross-process API requests |
| **Added** `idlePauseMilliseconds` | Quiet period after update completion for power-managed devices |

---

## Deprecated and Removed Fields

| Field | Introduced | Deprecated | Removed | Notes |
|-------|-----------|------------|---------|-------|
| `simulateUnhealthyState` | Pre-1.0 (0.8.0 preview) | v1.0 | v1.1 | Used only by the simulator platform layer, which was removed. Ignore this field in any current configuration. |

---

## Version-Specific Examples

### Minimal v1.0 Configuration

```json
{
  "schemaVersion": "1.0",
  "aduShellTrustedUsers": ["adu", "do"],
  "manufacturer": "Contoso",
  "model": "Smart-Box",
  "agents": [
    {
      "name": "main",
      "runas": "adu",
      "connectionSource": {
        "connectionType": "string",
        "connectionData": "HostName=hub.azure-devices.net;DeviceId=device01;SharedAccessKey=..."
      },
      "manufacturer": "Contoso",
      "model": "Smart-Box"
    }
  ]
}
```

### v1.1 Configuration with AIS and Protocol Selection

```json
{
  "schemaVersion": "1.1",
  "aduShellTrustedUsers": ["adu", "do"],
  "manufacturer": "Contoso",
  "model": "Smart-Box",
  "compatPropertyNames": "manufacturer,model",
  "iotHubProtocol": "mqtt",
  "downloadTimeoutInMinutes": 60,
  "agents": [
    {
      "name": "main",
      "runas": "adu",
      "connectionSource": {
        "connectionType": "AIS",
        "connectionData": "iotHubDeviceUpdate"
      },
      "manufacturer": "Contoso",
      "model": "Smart-Box",
      "additionalDeviceProperties": {
        "location": "US",
        "firmwareVersion": "2.0.0"
      }
    }
  ]
}
```

### v1.2 Configuration with X.509 and Idle Pause

```json
{
  "schemaVersion": "1.2",
  "aduShellTrustedUsers": ["adu", "do"],
  "manufacturer": "Contoso",
  "model": "Smart-Vacuum-v1",
  "iotHubProtocol": "mqtt",
  "idlePauseMilliseconds": 30000,
  "agents": [
    {
      "name": "main",
      "runas": "adu",
      "connectionSource": {
        "connectionType": "X509",
        "connectionData": "HostName=hub.azure-devices.net;DeviceId=device01;x509=true",
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

> For a full annotated example with all optional fields, see
> [Configuration Guide — Complete Example](configuration-guide.md#complete-example).

---

## Migration Notes

### Upgrading from v1.0 to v1.1

1. Change `"schemaVersion"` from `"1.0"` to `"1.1"`.
2. Optionally add `"iotHubProtocol": "mqtt"` (or `"mqtt/ws"` for WebSocket
   environments).
3. Optionally add `"downloadTimeoutInMinutes"` if you need a download deadline.
4. Remove `"simulateUnhealthyState"` if present — it is no longer recognized.

### Upgrading from v1.1 to v1.2

1. Change `"schemaVersion"` from `"1.1"` to `"1.2"`.
2. If using X.509 certificates, you may now set `"connectionType": "X509"`
   for explicit X.509 authentication instead of combining cert fields with
   `"string"` or `"AIS"` connection types.
3. Optionally add `"idlePauseMilliseconds"` for battery-powered or
   resource-constrained devices.

### Backward Compatibility

The agent parser treats all version-specific fields as optional and does not
reject a configuration based on its `schemaVersion` value. A v1.0 config file
will load successfully on an agent that supports v1.2 — the agent simply uses
defaults for any missing optional fields. However, using the latest schema
version is recommended to ensure all features are explicitly configured.

---

## See Also

- [Configuration Guide](configuration-guide.md) — field-by-field usage,
  connection-type examples, and complete reference
- [X.509 Authentication Guide](how-to-x509-authentication.md) — certificate
  provisioning, PKCS#11 setup, and troubleshooting
- [GetAduServiceStatus](GetAduServiceStatus.md) — idle-pause state diagram
  and SDK integration
