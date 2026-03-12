# Azure Device Update Agent Configuration Guide

The ADU agent is configured through a single JSON file. This guide documents every
field and connection method available as of v1.3.0-rc1.

## Configuration File Location

| Item | Value |
|------|-------|
| **Default path** | `/etc/adu/du-config.json` |
| **Override** | Set the `ADUC_CONF_FOLDER` environment variable to change the directory |

The agent reads and parses this file at startup via `ADUC_ConfigInfo_Init()`.

## Schema Version

All configurations must include `schemaVersion`. The current version is **`"1.2"`**.

```json
{
  "schemaVersion": "1.2"
}
```

## Connection Types

Each agent entry contains a `connectionSource` object that defines how the agent
authenticates with Azure IoT Hub. Three methods are supported.

### Connection String (SAS Token)

```json
"connectionSource": {
  "connectionType": "string",
  "connectionData": "HostName=<hub>.azure-devices.net;DeviceId=<device>;SharedAccessKey=<key>"
}
```

### Azure Identity Service (AIS)

Used with IoT Edge or managed-identity scenarios. `connectionData` is the AIS
principal name.

```json
"connectionSource": {
  "connectionType": "AIS",
  "connectionData": "iotHubDeviceUpdate"
}
```

### X.509 Certificate

Authenticates with a client certificate and private key. The connection string must
include `x509=true`.

```json
"connectionSource": {
  "connectionType": "X509",
  "connectionData": "HostName=<hub>.azure-devices.net;DeviceId=<device>;x509=true",
  "connectionX509CertFilePath": "/etc/adu/certs/client.pem",
  "connectionX509PrivateKeyFilePath": "/etc/adu/certs/client.key",
  "connectionX509CaCertFilePath": "/etc/adu/certs/ca.pem"
}
```

For PKCS#11 / HSM-backed keys, set `opensslEngine` to `"pkcs11"` and provide a
PKCS#11 URI as the private key path.

> **Note:** For the full X.509 provisioning walkthrough, see
> [how-to-x509-authentication.md](how-to-x509-authentication.md).

## Agent Configuration Fields

The `agents` array contains one or more agent definitions.

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | ✅ | Agent identifier (e.g. `"main"`) |
| `runas` | string | ✅ | OS user the agent runs as (e.g. `"adu"`) |
| `connectionSource` | object | ✅ | Connection method — see above |
| `manufacturer` | string | ✅ | Device manufacturer reported to the service |
| `model` | string | ✅ | Device model reported to the service |
| `additionalDeviceProperties` | object | ❌ | Custom key-value pairs reported alongside manufacturer/model |

Example with `additionalDeviceProperties`:

```json
{
  "name": "main",
  "runas": "adu",
  "connectionSource": { "connectionType": "string", "connectionData": "..." },
  "manufacturer": "Contoso",
  "model": "Smart-Box",
  "additionalDeviceProperties": {
    "location": "US",
    "firmwareVersion": "1.2.3"
  }
}
```

## Shell Configuration

`aduShellTrustedUsers` lists the OS users permitted to invoke ADU shell commands.
This field is **required**.

```json
"aduShellTrustedUsers": ["adu", "do"]
```

## Protocol Options

| Field | Type | Default | Values |
|-------|------|---------|--------|
| `iotHubProtocol` | string | `"mqtt"` | `"mqtt"`, `"mqtt/ws"` |

When omitted the agent defaults to MQTT. Use `"mqtt/ws"` (MQTT over WebSockets)
when outbound port 8883 is blocked.

## Power Management — `idlePauseMilliseconds`

*New in v1.3.0.*

Battery-powered or resource-constrained devices can set `idlePauseMilliseconds`
(top-level, unsigned integer) to introduce a quiet period after the agent finishes
an update and enters the Idle state. During this pause:

* Incoming cloud-to-device messages are silently dropped.
* `GetAduServiceStatus()` returns `ADUC_ServiceStatus_Paused`.
* The host application can put the device into a low-power mode.

Once the timer expires and any pending result reports are sent, the status
transitions to `ADUC_ServiceStatus_Idle` and the agent resumes normal operation.

```json
"idlePauseMilliseconds": 30000
```

Set to `0` (the default) to disable the pause.

> See [GetAduServiceStatus.md](GetAduServiceStatus.md) for the full state diagram
> and SDK integration details.

## Complete Example

```json
{
  "schemaVersion": "1.2",
  "aduShellTrustedUsers": ["adu", "do"],
  "iotHubProtocol": "mqtt",
  "manufacturer": "Contoso",
  "model": "Smart-Vacuum-v1",
  "idlePauseMilliseconds": 0,
  "agents": [
    {
      "name": "main",
      "runas": "adu",
      "connectionSource": {
        "connectionType": "string",
        "connectionData": "HostName=hub.azure-devices.net;DeviceId=device01;SharedAccessKey=..."
      },
      "manufacturer": "Contoso",
      "model": "Smart-Vacuum-v1",
      "additionalDeviceProperties": {
        "location": "US"
      }
    }
  ]
}
```

### Field Reference (all top-level keys)

| Field | Required | Default | Description |
|-------|----------|---------|-------------|
| `schemaVersion` | ✅ | — | Must be `"1.2"` |
| `manufacturer` | ✅ | — | Device manufacturer |
| `model` | ✅ | — | Device model |
| `aduShellTrustedUsers` | ✅ | — | Trusted OS users for shell commands |
| `agents` | ✅ | — | Array of agent configurations |
| `iotHubProtocol` | ❌ | `"mqtt"` | `"mqtt"` or `"mqtt/ws"` |
| `idlePauseMilliseconds` | ❌ | `0` | Quiet-period duration in ms |
| `downloadTimeoutInMinutes` | ❌ | `0` | Download timeout (0 = no timeout) |
| `compatPropertyNames` | ❌ | — | Backward-compat property names |
| `edgegatewayCertPath` | ❌ | — | Edge gateway certificate path |
| `aduShellFolder` | ❌ | build default | ADU shell binary directory |
| `dataFolder` | ❌ | `/var/lib/adu` | Agent data directory |
| `downloadsFolder` | ❌ | `<dataFolder>/downloads` | Download storage |
| `extensionsFolder` | ❌ | `<dataFolder>/extensions` | Extensions storage |
