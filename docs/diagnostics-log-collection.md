# Device Update Diagnostics Log Collection

The Azure Device Update (ADU) agent includes a comprehensive diagnostics component designed to collect and upload device logs to Azure cloud storage for troubleshooting and support purposes. This component operates independently but integrates seamlessly with the ADU agent infrastructure.

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Configuration](#configuration)
- [Usage Scenarios](#usage-scenarios)
- [Cloud Integration](#cloud-integration)
- [Security Considerations](#security-considerations)
- [Troubleshooting](#troubleshooting)
- [Best Practices](#best-practices)

## Overview

### Purpose

The diagnostics component serves a critical purpose in IoT device management:

- **Remote Log Collection**: Enables remote collection of device logs without physical access
- **Troubleshooting Support**: Provides detailed diagnostic information for support scenarios
- **Proactive Monitoring**: Allows for proactive identification of device issues
- **Compliance**: Helps meet regulatory requirements for device logging and auditing

### Key Features

- **Multi-Component Support**: Collects logs from multiple system components simultaneously
- **Configurable Collection**: Customizable log sources and collection limits
- **Secure Upload**: Uses Azure Blob Storage with SAS tokens for secure log transmission
- **Operation Tracking**: Prevents duplicate uploads and tracks operation completion
- **Size Management**: Configurable upload size limits to manage bandwidth and storage costs

## Architecture

### Component Structure

The diagnostics system consists of several interconnected components:

```
┌─────────────────────────┐
│   Azure Device Update   │
│        Agent            │
├─────────────────────────┤
│  Diagnostics Interface  │ ← IoT Hub PnP Component
│   (PnP Twin Component)  │
├─────────────────────────┤
│ Diagnostics Async Helper│ ← Workflow Management
├─────────────────────────┤
│  Diagnostics Workflow   │ ← Log Collection Logic
├─────────────────────────┤
│   Azure Blob Storage    │ ← Cloud Upload
│      File Upload        │
└─────────────────────────┘
```

### IoT Hub Integration

The diagnostics component integrates with Azure IoT Hub using the **IoT Plug and Play (PnP) Twin Component** pattern:

#### PnP Component Details
- **Component Name**: `diagnosticInformation`
- **Data Plane**: Shares the same IoT Hub connection as the ADU agent
- **Communication**: Uses device twin properties for command and response
- **Protocol**: MQTT or MQTT over WebSockets (same as ADU agent)

#### Twin Property Structure

**Cloud-to-Device Property** (`service`):
```json
{
  "diagnosticInformation": {
    "service": {
      "operationId": "12345678-1234-1234-1234-123456789012",
      "storageSasUrl": "https://mystorageaccount.blob.core.windows.net/logs/device123?sv=2021-01-01&sr=c&sig=..."
    }
  }
}
```

**Device-to-Cloud Property** (`agent`):

**According to Official DTDL Schema**:
```json
{
  "diagnosticInformation": {
    "agent": {
      "resultCode": 200,
      "extendedResultCode": 0,
      "operationId": "12345678-1234-1234-1234-123456789012"
    }
  }
}
```

**Current Implementation** (as of this agent version):
```json
{
  "diagnosticInformation": {
    "agent": {
      "resultCode": 200,
      "operationId": "12345678-1234-1234-1234-123456789012"
    }
  }
}
```

**⚠️ Implementation Note**: While the official DTDL schema defines `extendedResultCode` as part of the `agent` property, the current ADU agent implementation only sends `resultCode` and `operationId`. The `extendedResultCode` field is defined in the header files but not currently used in the reporting function.

#### Result Codes

The `resultCode` field indicates the outcome of the diagnostics operation:

| Code | Name | Description |
|------|------|-------------|
| `200` | `Diagnostics_Result_Success` | Upload of all component logs was successful |
| `0` | `Diagnostics_Result_Failure` | Generic failure not related to specific workflow issues |
| `-1` | `Diagnostics_Result_NoLogsFound` | No logs were found for a component |
| `-2` | `Diagnostics_Result_UploadFailed` | Uploading the files failed for some component |
| `-3` | `Diagnostics_Result_ContainerCreateFailed` | Unable to create the container for log upload |
| `-4` | `Diagnostics_Result_BadCredential` | The SAS credential sent to device is improperly formed |
| `-5` | `Diagnostics_Result_NoDiagnosticsComponents` | Diagnostics configuration doesn't contain any components |
| `-6` | `Diagnostics_Result_NoOperationId` | Cloud-to-device message contains no operation ID |
| `-7` | `Diagnostics_Result_NoSasCredential` | Cloud-to-device message contains no SAS credential |

### Workflow Process

1. **Service Request**: Azure IoT Hub sends a diagnostics request via device twin
2. **Operation Validation**: Component checks for duplicate operations using operation ID
3. **Log Collection**: Gathers logs from configured component paths
4. **File Preparation**: Creates compressed archive of collected logs
5. **Secure Upload**: Uploads to Azure Blob Storage using provided SAS URL
6. **Status Reporting**: Reports completion status back to IoT Hub via device twin

## Configuration

### Configuration File Location

The diagnostics component uses its own configuration file separate from the main ADU configuration:

- **Default Path**: `/etc/adu/du-diagnostics-config.json`
- **Build-time Configurable**: Path can be modified via `DIAGNOSTICS_CONFIG_FILE_PATH` CMake variable

### Configuration File Format

```json
{
  "logComponents": [
    {
      "componentName": "adu",
      "logPath": "/var/log/adu/"
    },
    {
      "componentName": "delivery-optimization",
      "logPath": "/var/cache/do/"
    }
  ],
  "maxKilobytesToUploadPerLogPath": 50
}
```

### Configuration Parameters

#### `logComponents` Array
Defines which component logs to collect:

| Field | Type | Description | Example |
|-------|------|-------------|---------|
| `componentName` | String | Human-readable identifier for the log source | `"adu"`, `"delivery-optimization"` |
| `logPath` | String | Absolute path to log **directory** (not individual files) | `"/var/log/adu/"`, `"/var/cache/do"` |

**Important**: The `logPath` must be a **directory path**, not a file path. The component will scan the directory for log files and collect the most recent ones up to the size limit.

#### `maxKilobytesToUploadPerLogPath`
- **Type**: Integer (1-100,000)
- **Unit**: Kilobytes
- **Default Cap**: 100,000 KB (100 MB)
- **Purpose**: Limits upload size per log path to manage bandwidth and storage costs

### Example Configurations

#### Minimal Configuration
```json
{
  "logComponents": [
    {
      "componentName": "adu",
      "logPath": "/var/log/adu/"
    }
  ],
  "maxKilobytesToUploadPerLogPath": 10
}
```

#### Comprehensive Configuration
```json
{
  "logComponents": [
    {
      "componentName": "adu-agent",
      "logPath": "/var/log/adu/"
    },
    {
      "componentName": "delivery-optimization",
      "logPath": "/var/cache/do/"
    },
    {
      "componentName": "system-logs",
      "logPath": "/var/log/"
    },
    {
      "componentName": "application-logs",
      "logPath": "/opt/myapp/logs/"
    }
  ],
  "maxKilobytesToUploadPerLogPath": 100
}
```

## Usage Scenarios

### 1. Support Case Investigation

**Scenario**: Device experiencing update failures
```json
{
  "logComponents": [
    {
      "componentName": "adu-full",
      "logPath": "/var/log/adu/"
    },
    {
      "componentName": "system-critical",
      "logPath": "/var/log/"
    }
  ],
  "maxKilobytesToUploadPerLogPath": 200
}
```

### 2. Proactive Device Monitoring

**Scenario**: Regular health check collection
```json
{
  "logComponents": [
    {
      "componentName": "adu-summary",
      "logPath": "/var/log/adu/"
    },
    {
      "componentName": "system-health",
      "logPath": "/var/log/"
    }
  ],
  "maxKilobytesToUploadPerLogPath": 25
}
```

### 3. Application-Specific Diagnostics

**Scenario**: Custom application troubleshooting
```json
{
  "logComponents": [
    {
      "componentName": "iot-edge",
      "logPath": "/var/log/iotedge/"
    },
    {
      "componentName": "docker-containers",
      "logPath": "/var/lib/docker/containers/"
    }
  ],
  "maxKilobytesToUploadPerLogPath": 150
}
```

## Cloud Integration

### Azure IoT Hub Setup

#### 1. Device Twin Configuration

The diagnostics component integrates with Azure IoT Hub as a PnP component named `diagnosticInformation`.

**📚 Official DTDL Model**: [dtmi:azure:iot:diagnosticInformation;1](https://github.com/Azure/iot-plugandplay-models/blob/main/dtmi/azure/iot/diagnosticinformation-1.json)

Based on the official DTDL schema, the component defines:
- **Component name**: `diagnosticInformation`
- **Cloud-to-device property**: `service` (contains `operationId` and `storageSasUrl`)
- **Device-to-cloud property**: `agent` (contains `resultCode`, `extendedResultCode`, and `operationId`)

For the complete device twin model that includes the main ADU component, refer to:
- **ADU Contract Model ID**: `dtmi:azure:iot:deviceUpdateContractModel;3` (found in the source code)
- **[Device Update agent overview](https://learn.microsoft.com/en-us/azure/iot-hub-device-update/device-update-agent-overview)** - Official agent architecture documentation

**Implementation Note**: The diagnostics component is built into the ADU agent and will be available automatically when the agent connects to IoT Hub. No additional device twin model configuration is required specifically for diagnostics.

#### 2. Initiating Log Collection

Send a diagnostic request via device twin desired properties. This follows the standard process documented in Microsoft's official guide.

**📚 Official Documentation**:
- **[Remotely collect diagnostic logs from devices using Device Update for IoT Hub](https://learn.microsoft.com/en-us/azure/iot-hub-device-update/device-update-log-collection)** - Complete step-by-step guide
- **[Device Update diagnostics overview](https://learn.microsoft.com/en-us/azure/iot-hub-device-update/device-update-diagnostics)** - Feature overview and concepts
- **[Diagnostic Information Interface DTDL Model](https://github.com/Azure/iot-plugandplay-models/blob/main/dtmi/azure/iot/diagnosticinformation-1.json)** - Official schema definition

**Using Azure CLI** (for advanced scenarios):
```bash
# Using Azure CLI to set device twin desired properties directly
# Note: The official recommendation is to use the Device Update Portal UI or REST APIs
az iot hub device-twin update \
  --device-id "myDevice" \
  --hub-name "myIoTHub" \
  --set properties.desired.diagnosticInformation.service='{
    "operationId": "12345678-1234-1234-1234-123456789012",
    "storageSasUrl": "https://mystorageaccount.blob.core.windows.net/logs/device123?sv=2021-01-01&sr=c&sig=..."
  }'
```

**⚠️ Important**: The recommended approach is to use the **Device Update Portal UI** or **Device Update REST APIs** rather than directly manipulating device twin properties, as documented in the official Microsoft guide above.

### Azure Storage Account Setup

#### 1. Create Storage Account
```bash
# Create storage account
az storage account create \
  --name "devicediagnostics" \
  --resource-group "myResourceGroup" \
  --location "eastus" \
  --sku "Standard_LRS"

# Create container for device logs
az storage container create \
  --name "device-logs" \
  --account-name "devicediagnostics"
```

#### 2. Generate SAS Token
```bash
# Generate SAS token with appropriate permissions
az storage container generate-sas \
  --account-name "devicediagnostics" \
  --name "device-logs" \
  --permissions "acw" \
  --expiry "2024-12-31T23:59:59Z" \
  --output tsv
```

### Monitoring and Alerts

#### 1. Monitor Upload Success
```bash
# Check device twin reported properties for diagnostics status
az iot hub device-twin show \
  --device-id "myDevice" \
  --hub-name "myIoTHub" \
  --query "properties.reported.diagnosticInformation.agent"
```

#### 2. Set Up Azure Monitor Alerts
```json
{
  "alertRule": {
    "name": "DiagnosticsUploadFailure",
    "condition": {
      "field": "properties.reported.diagnosticInformation.agent.resultCode",
      "operator": "NotEquals",
      "value": 200
    },
    "action": {
      "type": "email",
      "recipients": ["support@company.com"]
    }
  }
}
```

## Security Considerations

### 1. Access Control

- **SAS Token Permissions**: Use minimal required permissions (Add, Create, Write)
- **Token Expiration**: Set appropriate expiration times for SAS tokens
- **Storage Access**: Restrict storage account access to authorized personnel only

### 2. Data Privacy

- **Log Sanitization**: Ensure logs don't contain sensitive information
- **Encryption**: Logs are encrypted in transit (HTTPS) and at rest (Azure Storage)
- **Access Logging**: Enable storage account access logging for audit trails

### 3. Network Security

- **Firewall Rules**: Configure storage account firewall if needed
- **Private Endpoints**: Consider using private endpoints for enhanced security
- **Device Authentication**: Leverages existing IoT Hub device authentication

### 4. Configuration Security

```bash
# Secure configuration file permissions
sudo chown root:adu /etc/adu/du-diagnostics-config.json
sudo chmod 640 /etc/adu/du-diagnostics-config.json
```

## Troubleshooting

### Common Issues

#### 1. Configuration File Not Found
```
Error: DiagnosticsConfigUtils_Init failed
```

**Solution**:
```bash
# Check if configuration file exists
ls -la /etc/adu/du-diagnostics-config.json

# Create default configuration if missing
sudo mkdir -p /etc/adu
sudo tee /etc/adu/du-diagnostics-config.json > /dev/null << 'EOF'
{
  "logComponents": [
    {
      "componentName": "adu",
      "logPath": "/var/log/adu/"
    }
  ],
  "maxKilobytesToUploadPerLogPath": 50
}
EOF
```

#### 2. Invalid Log Path Configuration
```
Error: Unable to access log path or no logs found
```

**Common Mistake**: Specifying a file path instead of a directory path.

**Incorrect Example**:
```json
{
  "componentName": "system",
  "logPath": "/var/log/syslog"  ← This is a FILE, not a directory
}
```

**Correct Example**:
```json
{
  "componentName": "system",
  "logPath": "/var/log/"  ← This is a DIRECTORY containing syslog and other files
}
```

**Solution**: Always use directory paths. The component will automatically scan the directory for log files.

#### 3. Upload Authentication Failure
```
Error: Failed to upload logs to Azure Storage
```

**Solutions**:
- Verify SAS token is not expired
- Check SAS token permissions (must include Add, Create, Write)
- Ensure storage account is accessible from device network

#### 3. Log Path Access Issues
```
Error: Cannot access log path /var/log/adu/
```

**Solutions**:
```bash
# Check directory permissions
ls -la /var/log/adu/

# Fix permissions if needed
sudo chown -R adu:adu /var/log/adu/
sudo chmod -R 755 /var/log/adu/
```

#### 4. Operation Already Completed
```
Info: Operation ID already completed, skipping
```

**Explanation**: This is normal behavior to prevent duplicate uploads. The system tracks completed operations to avoid reprocessing.

### Diagnostic Commands

#### Check Diagnostics Component Status
```bash
# Check if diagnostics component is loaded
journalctl -u deviceupdate-agent.service | grep -i diagnostic

# Check configuration parsing
sudo journalctl -u deviceupdate-agent.service --since "1 hour ago" | grep -i "DiagnosticsConfigUtils"
```

#### Validate Configuration File
```bash
# Validate JSON syntax
python3 -m json.tool /etc/adu/du-diagnostics-config.json

# Check required fields
jq '.logComponents[].componentName, .logComponents[].logPath, .maxKilobytesToUploadPerLogPath' /etc/adu/du-diagnostics-config.json
```

#### Test Storage Connectivity
```bash
# Test SAS URL accessibility
curl -I "https://mystorageaccount.blob.core.windows.net/logs/?sv=2021-01-01&sr=c&sig=..."
```

### Log Analysis

#### Enable Verbose Logging
```bash
# Start agent with verbose logging
sudo /usr/bin/AducIotAgent --log-level 0

# Or modify systemd service
sudo systemctl edit deviceupdate-agent.service
```

Add in the override file:
```ini
[Service]
ExecStart=
ExecStart=/usr/bin/AducIotAgent --log-level 0
```

## Best Practices

### 1. Configuration Management

- **Version Control**: Store configuration files in version control
- **Environment-Specific**: Use different configurations for dev/staging/production
- **Validation**: Validate configuration changes before deployment

### 2. Log Collection Strategy

- **Selective Collection**: Only collect logs relevant to current issues
- **Size Management**: Balance diagnostic value with upload costs
- **Rotation Awareness**: Consider log rotation when setting paths

### 3. Operational Excellence

- **Monitoring**: Set up monitoring for failed uploads
- **Automation**: Automate diagnostic collection for known issues
- **Documentation**: Document custom log sources and their purposes

### 4. Storage Management

- **Lifecycle Policies**: Set up Azure Storage lifecycle policies for cost optimization
- **Organization**: Use consistent naming conventions for uploaded files
- **Cleanup**: Regularly clean up old diagnostic files

### Example Lifecycle Policy
```json
{
  "rules": [
    {
      "name": "DiagnosticLogRetention",
      "type": "Lifecycle",
      "definition": {
        "filters": {
          "blobTypes": ["blockBlob"],
          "prefixMatch": ["device-logs/"]
        },
        "actions": {
          "baseBlob": {
            "tierToCool": {"daysAfterModificationGreaterThan": 30},
            "tierToArchive": {"daysAfterModificationGreaterThan": 90},
            "delete": {"daysAfterModificationGreaterThan": 365}
          }
        }
      }
    }
  ]
}
```

### 5. Security Best Practices

- **Least Privilege**: Grant minimal necessary permissions
- **Regular Rotation**: Rotate SAS tokens regularly
- **Audit Trail**: Maintain audit trails for diagnostic requests
- **Data Classification**: Classify diagnostic data appropriately

## Integration with ADU Agent

### Relationship to ADU Agent

The diagnostics component operates as an **integrated but independent** component within the ADU agent:

#### Shared Infrastructure
- **IoT Hub Connection**: Uses the same MQTT/IoT Hub connection as ADU updates
- **Device Authentication**: Leverages ADU agent's device authentication
- **PnP Framework**: Integrates with the ADU agent's PnP component system
- **Logging System**: Uses the same logging infrastructure

#### Independent Operation
- **Separate Configuration**: Has its own configuration file and settings
- **Independent Workflows**: Log collection operates independently of update workflows
- **Dedicated Component**: Runs as a separate PnP component (`diagnosticInformation`)
- **Async Processing**: Handles log upload asynchronously without blocking updates

#### Deployment Considerations
- **Single Binary**: Compiled into the same `AducIotAgent` binary
- **Shared Dependencies**: Benefits from ADU agent's Azure SDK dependencies
- **Service Management**: Managed through the same systemd service
- **Update Compatibility**: Diagnostics functionality updates with agent updates

This architecture ensures that diagnostic capabilities are readily available whenever the ADU agent is running, while maintaining clear separation of concerns between update operations and diagnostic collection.
