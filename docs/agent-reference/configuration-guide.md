# Configuration Guide

This guide covers the complete Device Update agent configuration system, including schema versions, authentication methods, and advanced features.

## Configuration Overview

The Device Update agent uses a JSON configuration file located at `/etc/adu/du-config.json`. The configuration supports multiple schema versions and provides extensive customization options.

## Configuration Schema 1.2

Schema 1.2 is the recommended version for new deployments, providing enhanced authentication, power management, and SDK integration.

### Basic Configuration Structure

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
        "connectionData": "HostName=hub.azure-devices.net;DeviceId=device01;SharedAccessKey=..."
      },
      "manufacturer": "Contoso",
      "model": "Smart-Device-v1"
    }
  ]
}
```

## Global Configuration Properties

### Core Settings

| Property | Type | Required | Description |
|----------|------|----------|-------------|
| `schemaVersion` | string | Yes | Configuration schema version ("1.1" or "1.2") |
| `aduShellTrustedUsers` | array | Yes | Users allowed to execute privileged operations |
| `iotHubProtocol` | string | Yes | Communication protocol ("mqtt" or "mqtt-ws") |
| `agents` | array | Yes | Agent instance configurations |

### Schema 1.2 Enhancements

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `idlePauseMilliseconds` | number | 0 | Pause duration between agent loops (power management) |
| `apiRequestFifoPath` | string | auto | FIFO path for SDK API communication |

### Additional Global Properties

| Property | Type | Required | Description |
|----------|------|----------|-------------|
| `manufacturer` | string | Yes | Device manufacturer for compatibility checks |
| `model` | string | Yes | Device model for compatibility checks |
| `compatPropertyNames` | string | No | Comma-separated compatibility property names (default: "manufacturer,model") |
| `downloadTimeoutInMinutes` | number | No | Download timeout override (default: 480 minutes) |
| `edgegatewayCertPath` | string | No | Path to IoT Edge gateway certificate |
| `aduShellFolder` | string | No | Custom path to adu-shell binary folder |
| `dataFolder` | string | No | Custom path to agent data folder |
| `extensionsFolder` | string | No | Custom path to extensions folder |
| `downloadsFolder` | string | No | Custom path to downloads folder |

### Power Management (Schema 1.2+)

```json
{
  "schemaVersion": "1.2",
  "idlePauseMilliseconds": 30000,
  "agents": [...]
}
```

**Power Management Benefits**:
- Reduced CPU usage during idle periods
- Battery life optimization for portable devices
- Configurable responsiveness vs. power consumption
- Integration with SDK for application-aware power management

## Agent Configuration

### Required Agent Properties

| Property | Type | Required | Description |
|----------|------|----------|-------------|
| `name` | string | Yes | Unique agent identifier |
| `runas` | string | Yes | System user for agent execution |
| `connectionSource` | object | Yes | IoT Hub connection configuration |
| `manufacturer` | string | Yes | Device manufacturer identifier |
| `model` | string | Yes | Device model identifier |

### Optional Agent Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `additionalDeviceProperties` | object | {} | Custom device properties |

**Removed Properties**:
- `installedCriteria` - This property is no longer used in agent configuration. Update handlers now determine installation status through their own mechanisms.

## Authentication Methods

### 1. Connection String Authentication

**Use Case**: Development, testing, simple deployments

```json
{
  "connectionSource": {
    "connectionType": "string",
    "connectionData": "HostName=hub.azure-devices.net;DeviceId=device01;SharedAccessKey=abc123..."
  }
}
```

**Security Considerations**:
- Shared access key in configuration file
- Suitable for development and testing
- Consider key rotation policies

### 2. X.509 Certificate Authentication (Schema 1.2+)

**Use Case**: Production deployments, enhanced security, HSM integration

```json
{
  "connectionSource": {
    "connectionType": "X509",
    "connectionData": "HostName=hub.azure-devices.net;DeviceId=device01;x509=true",
    "connectionX509CertFilePath": "/etc/adu/certs/client.pem",
    "connectionX509PrivateKeyFilePath": "/etc/adu/certs/client.key",
    "connectionX509CaCertFilePath": "/etc/adu/certs/ca.pem"
  }
}
```

**Certificate Requirements**:
- Device certificate (or thumbprints) must be registered in IoT Hub
- Certificate chain validation to trusted CA
- Private key must be accessible to agent user
- Support for PKCS#11 HSM integration

### 3. Azure Identity Service (AIS)

**Use Case**: IoT Edge scenarios, managed identity

```json
{
  "connectionSource": {
    "connectionType": "AIS",
    "connectionData": "HostName=hub.azure-devices.net;DeviceId=device01;ModuleId=AzureDeviceUpdateAgent"
  }
}
```

**Requirements**:
- Running in IoT Edge environment
- Module identity configured in IoT Edge
- Automatic token management by IoT Edge

## X.509 Certificate Setup

### Certificate File Structure

```bash
/etc/adu/certs/
├── client.pem          # Device certificate
├── client.key          # Private key
├── ca.pem             # CA certificate chain
└── README.md          # Certificate documentation
```

### Certificate Generation Example

```bash
# Generate private key
openssl genrsa -out client.key 2048

# Create certificate signing request
openssl req -new -key client.key -out client.csr -subj "/CN=device01"

# Generate self-signed certificate (for testing)
openssl x509 -req -days 365 -in client.csr -signkey client.key -out client.pem

# Set proper permissions
sudo chown adu:adu /etc/adu/certs/*
sudo chmod 600 /etc/adu/certs/client.key
sudo chmod 644 /etc/adu/certs/client.pem
sudo chmod 644 /etc/adu/certs/ca.pem
```

### HSM Integration (Future Plan)

For hardware security modules, configure PKCS#11:

```json
{
  "connectionSource": {
    "connectionType": "X509",
    "connectionData": "HostName=hub.azure-devices.net;DeviceId=device01;x509=true",
    "connectionX509CertFilePath": "pkcs11:token=MyToken;object=device-cert",
    "connectionX509PrivateKeyFilePath": "pkcs11:token=MyToken;object=device-key",
    "connectionX509CaCertFilePath": "/etc/adu/certs/ca.pem"
  }
}
```

## Multi-Agent Configuration (Unsupported)

For complex scenarios, configure multiple agent instances:

```json
{
  "schemaVersion": "1.2",
  "aduShellTrustedUsers": ["adu", "do"],
  "iotHubProtocol": "mqtt",
  "agents": [
    {
      "name": "host",
      "runas": "adu",
      "connectionSource": {
        "connectionType": "string",
        "connectionData": "HostName=hub.azure-devices.net;DeviceId=gateway01;SharedAccessKey=..."
      },
      "manufacturer": "Contoso",
      "model": "Gateway-v1"
    },
    {
      "name": "sensor1",
      "runas": "adu",
      "connectionSource": {
        "connectionType": "string",
        "connectionData": "HostName=hub.azure-devices.net;DeviceId=sensor01;SharedAccessKey=..."
      },
      "manufacturer": "Contoso",
      "model": "Sensor-v1"
    }
  ]
}
```

## Advanced Configuration Options

### Custom Device Properties

```json
{
  "agents": [
    {
      "name": "main",
      "additionalDeviceProperties": {
        "location": "Building-A",
        "firmware": "1.2.3",
        "capabilities": ["wifi", "bluetooth", "cellular"]
      }
    }
  ]
}
```

### SDK Integration (Schema 1.2+)

```json
{
  "schemaVersion": "1.2",
  "apiRequestFifoPath": "/var/lib/adu/api.fifo",
  "agents": [...]
}
```

**SDK API Features**:
- External application integration
- Status monitoring and notifications
- Power management coordination
- Update scheduling and control

## Configuration Validation

### Runtime Validation

The agent automatically validates configuration on startup and will log errors if the configuration is invalid. Configuration errors will prevent the agent from starting successfully.

### Configuration Testing

```bash
# Validate JSON syntax
cat /etc/adu/du-config.json | python3 -m json.tool

# Test agent startup and check for configuration errors
sudo systemctl start deviceupdate-agent
sudo journalctl -u deviceupdate-agent --no-pager -l | grep -i error

# Check file permissions
ls -la /etc/adu/du-config.json
```

## Migration from Schema 1.1 to 1.2

### Key Differences

| Feature | Schema 1.1 | Schema 1.2 |
|---------|------------|------------|
| X.509 Auth | Limited | Full support |
| Power Management | No | Yes |
| SDK Integration | No | Yes |
| Enhanced Logging | No | Yes |

### Migration Steps

1. **Backup current configuration**:
   ```bash
   sudo cp /etc/adu/du-config.json /etc/adu/du-config.json.backup
   ```

2. **Update schema version**:
   ```json
   {
     "schemaVersion": "1.2",
     ...
   }
   ```

3. **Add new optional properties**:
   ```json
   {
     "schemaVersion": "1.2",
     "idlePauseMilliseconds": 60000,
     ...
   }
   ```

4. **Upgrade X.509 configuration** (if using certificates):
   ```json
   {
     "connectionSource": {
       "connectionType": "X509",
       "connectionData": "HostName=hub.azure-devices.net;DeviceId=device01;x509=true",
       "connectionX509CertFilePath": "/etc/adu/certs/client.pem",
       "connectionX509PrivateKeyFilePath": "/etc/adu/certs/client.key",
       "connectionX509CaCertFilePath": "/etc/adu/certs/ca.pem"
     }
   }
   ```

5. **Test new configuration**:
   ```bash
   sudo systemctl stop deviceupdate-agent
   sudo -u adu AducIotAgent --config-file /etc/adu/du-config.json --validate-config
   sudo systemctl start deviceupdate-agent
   ```

## Configuration Security

### File Permissions

```bash
# Secure configuration file
sudo chown root:adu /etc/adu/du-config.json
sudo chmod 640 /etc/adu/du-config.json

# Secure certificate directory
sudo chown -R root:adu /etc/adu/certs/
sudo chmod 755 /etc/adu/certs/
sudo chmod 600 /etc/adu/certs/*.key
sudo chmod 644 /etc/adu/certs/*.pem
```

### Sensitive Data Protection

**Connection Strings**:
- Avoid logging connection strings
- Use environment variables for sensitive data
- Implement key rotation procedures

**Certificates**:
- Protect private keys with appropriate permissions
- Use HSM for production deployments
- Implement certificate lifecycle management

## Troubleshooting Configuration Issues

### Connection Problems

```bash
# Test connectivity
curl -I https://your-hub.azure-devices.net

# Verify device identity
az iot hub device-identity show --hub-name your-hub --device-id your-device

# Check certificate validity
openssl x509 -in /etc/adu/certs/client.pem -text -noout
```

### Permission Issues

```bash
# Check user exists
id adu

# Verify file ownership
ls -la /etc/adu/

# Test file access
sudo -u adu cat /etc/adu/du-config.json
```

### Agent Startup Issues

```bash
# View startup logs
sudo journalctl -u deviceupdate-agent -f

# Check configuration syntax
python3 -c "import json; print(json.load(open('/etc/adu/du-config.json')))"

# Test agent with verbose logging
sudo -u adu AducIotAgent --config-file /etc/adu/du-config.json --log-level verbose
```

## Configuration Examples

### Production X.509 Setup

```json
{
  "schemaVersion": "1.2",
  "aduShellTrustedUsers": ["adu", "do"],
  "iotHubProtocol": "mqtt",
  "idlePauseMilliseconds": 30000,
  "manufacturer": "Contoso",
  "model": "Industrial-Controller-v2",
  "agents": [
    {
      "name": "main",
      "runas": "adu",
      "connectionSource": {
        "connectionType": "X509",
        "connectionData": "HostName=production-hub.azure-devices.net;DeviceId=prod-device-001;x509=true",
        "connectionX509CertFilePath": "/etc/adu/certs/device.pem",
        "connectionX509PrivateKeyFilePath": "/etc/adu/certs/device.key",
        "connectionX509CaCertFilePath": "/etc/adu/certs/ca-chain.pem"
      },
      "manufacturer": "Contoso",
      "model": "Industrial-Controller-v2",
      "additionalDeviceProperties": {
        "location": "Factory-Floor-A",
        "firmwareVersion": "2.1.0",
        "capabilities": ["ethernet", "rs485", "canbus"]
      }
    }
  ]
}
```

### Development Setup

```json
{
  "schemaVersion": "1.2",
  "aduShellTrustedUsers": ["adu", "do"],
  "iotHubProtocol": "mqtt",
  "idlePauseMilliseconds": 5000,
  "agents": [
    {
      "name": "dev",
      "runas": "adu",
      "connectionSource": {
        "connectionType": "string",
        "connectionData": "HostName=dev-hub.azure-devices.net;DeviceId=dev-device;SharedAccessKey=development-key"
      },
      "manufacturer": "Developer",
      "model": "TestDevice"
    }
  ]
}
```

## Next Steps

Now that you understand agent configuration:

1. **[Architecture Overview](architecture-overview.md)** - Understand the system architecture
2. **[Authentication Setup](authentication-setup.md)** - Detailed authentication configuration
3. **[Agent Workflow](agent-workflow.md)** - Learn how the agent processes updates
4. **[SDK Integration](sdk-integration.md)** - Integrate with external applications
5. **[Monitoring & Diagnostics](monitoring-diagnostics.md)** - Set up monitoring and logging
