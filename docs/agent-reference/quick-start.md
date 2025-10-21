# Quick Start Guide

Get the Device Update agent running on your device in 15 minutes with this step-by-step guide.

## Prerequisites

Before starting, ensure you have:

- **Linux Device**: Ubuntu 18.04+, Debian 10+, or equivalent
- **Azure Subscription**: With IoT Hub created
- **Development Tools**: Build dependencies installed
- **Internet Access**: For downloading packages and connecting to Azure

## Step 1: Set Up Azure Resources

### Create IoT Hub and Device

1. **Create an IoT Hub** (if you don't have one):
   ```bash
   az iot hub create --name MyIoTHub --resource-group MyResourceGroup --sku S1
   ```

2. **Create a device identity**:
   ```bash
   az iot hub device-identity create --hub-name MyIoTHub --device-id my-test-device
   ```

3. **Get the connection string**:
   ```bash
   az iot hub device-identity connection-string show --hub-name MyIoTHub --device-id my-test-device --output table
   ```

### Create Device Update Instance

1. **Create Device Update account**:
   ```bash
   az iot du account create --account MyDUAccount --resource-group MyResourceGroup
   ```

2. **Create Device Update instance**:
   ```bash
   az iot du instance create --account MyDUAccount --instance MyDUInstance --iothub-ids /subscriptions/{subscription-id}/resourceGroups/MyResourceGroup/providers/Microsoft.Devices/IotHubs/MyIoTHub
   ```

## Step 2: Install Build Dependencies

### Ubuntu/Debian Systems

```bash
# Update package list
sudo apt update

# Install build tools
sudo apt install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    ninja-build \
    libssl-dev \
    libcurl4-openssl-dev \
    uuid-dev

# Install optional dependencies
sudo apt install -y \
    libglib2.0-dev \
    libxml2-dev \
    libgsoapssl-dev
```

### Create Build User

```bash
# Create adu user for running the agent
sudo useradd -m -s /bin/bash adu
sudo useradd -m -s /bin/bash do

# Add users to necessary groups
sudo usermod -aG adm adu
sudo usermod -aG dialout adu
```

## Step 3: Build the Agent

### Clone and Build

```bash
# Clone the repository
git clone https://github.com/Azure/iot-hub-device-update.git
cd iot-hub-device-update

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DADUC_BUILD_UNIT_TESTS=OFF \
    -DADUC_PLATFORM_LAYER=linux

# Build the agent
cmake --build . --target install
```

### Build Output

After successful build, you'll find:

- **Agent binary**: `build/bin/AducIotAgent`
- **Shell utility**: `build/bin/adu-shell`
- **Configuration**: `build/src/agent/adu-conf/du-config.json`
- **Service file**: `daemon/deviceupdate-agent.service`

## Step 4: Configure the Agent

### Create Configuration

Create `/etc/adu/du-config.json` with your device details:

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
        "connectionData": "HostName=MyIoTHub.azure-devices.net;DeviceId=my-test-device;SharedAccessKey=YOUR_DEVICE_KEY"
      },
      "manufacturer": "Contoso",
      "model": "VirtualDevice"
    }
  ]
}
```

**Replace**:
- `MyIoTHub.azure-devices.net` with your IoT Hub hostname
- `my-test-device` with your device ID
- `YOUR_DEVICE_KEY` with the device's primary key
- `Contoso` and `VirtualDevice` with your manufacturer and model

### Set Permissions

```bash
# Create configuration directory
sudo mkdir -p /etc/adu

# Copy and set permissions
sudo cp du-config.json /etc/adu/
sudo chown root:adu /etc/adu/du-config.json
sudo chmod 640 /etc/adu/du-config.json

# Create additional directories
sudo mkdir -p /var/lib/adu
sudo chown adu:adu /var/lib/adu

sudo mkdir -p /var/log/adu
sudo chown adu:adu /var/log/adu
```

## Step 5: Install and Start the Service

### Install Service Files

```bash
# Copy binaries to system location
sudo cp build/bin/AducIotAgent /usr/bin/
sudo cp build/bin/adu-shell /usr/bin/
sudo chmod +x /usr/bin/AducIotAgent
sudo chmod +x /usr/bin/adu-shell

# Copy service file
sudo cp daemon/deviceupdate-agent.service /etc/systemd/system/
sudo systemctl daemon-reload
```

### Start the Agent

```bash
# Enable and start the service
sudo systemctl enable deviceupdate-agent
sudo systemctl start deviceupdate-agent

# Check service status
sudo systemctl status deviceupdate-agent
```

## Step 6: Verify Installation

### Check Agent Status

```bash
# View service logs
sudo journalctl -u deviceupdate-agent -f

# Check if agent is connected
sudo journalctl -u deviceupdate-agent | grep "Connected to IoT Hub"
```

### Test Communication

1. **Check device twin** in Azure IoT Explorer or portal
2. **Verify device properties** are being reported
3. **Look for agent information** in device twin reported properties

Expected device properties:
```json
{
  "deviceUpdate": {
    "agent": {
      "state": "Idle",
      "lastInstallResult": {
        "resultCode": 0,
        "extendedResultCode": 0
      }
    }
  }
}
```

## Step 7: Create Your First Update

### Prepare Update Package

For this quick start, we'll create a simple script update:

1. **Create update script**:
   ```bash
   # create-update.sh
   #!/bin/bash
   echo "Hello from Device Update!" > /tmp/adu-test.txt
   echo "Update completed at $(date)" >> /tmp/adu-test.txt
   ```

2. **Create update manifest**:
   ```json
   {
     "manifestVersion": "5.0",
     "updateId": {
       "provider": "Contoso",
       "name": "VirtualDevice",
       "version": "1.0.0"
     },
     "compatibility": [
       {
         "manufacturer": "Contoso",
         "model": "VirtualDevice"
       }
     ],
     "instructions": {
       "steps": [
         {
           "handler": "microsoft/script:1",
           "files": ["create-update.sh"],
           "handlerProperties": {
             "scriptFileName": "create-update.sh"
           }
         }
       ]
     },
     "files": {
       "create-update.sh": {
         "fileName": "create-update.sh",
         "sizeInBytes": 150,
         "hashes": {
           "sha256": "abcd1234..."
         }
       }
     }
   }
   ```

### Deploy Update

1. **Upload to Azure Storage** and get URL
2. **Create deployment** in Device Update service
3. **Monitor progress** in Azure portal or IoT Explorer

## Troubleshooting

### Common Issues

**Agent won't start**:
```bash
# Check configuration syntax
cat /etc/adu/du-config.json | python3 -m json.tool

# Check file permissions
ls -la /etc/adu/du-config.json

# View detailed logs
sudo journalctl -u deviceupdate-agent --no-pager -l
```

**Connection problems**:
```bash
# Test IoT Hub connectivity
curl -I https://MyIoTHub.azure-devices.net

# Verify device credentials
az iot hub device-identity connection-string show --hub-name MyIoTHub --device-id my-test-device
```

**Permission errors**:
```bash
# Check user exists
id adu
id do

# Verify directory permissions
ls -la /var/lib/adu
ls -la /var/log/adu
```

### Log Analysis

**Key log patterns to look for**:
- `Successfully connected to IoT Hub` - Connection established
- `Received PnP command` - Device Update commands received
- `Workflow completed` - Update workflow finished
- `ERROR` or `WARN` - Issues that need attention

### Diagnostic Tools

Run the diagnostic script for comprehensive system check:
```bash
# Use the built-in diagnostic tool
sudo /path/to/scripts/adu-diag.sh
```

## Next Steps

Now that you have the agent running:

1. **[Configuration Guide](configuration-guide.md)** - Learn about advanced configuration options
2. **[Agent Workflow](agent-workflow.md)** - Understand how updates are processed
3. **[Extension Development](extension-development.md)** - Create custom update handlers
4. **[Authentication Setup](authentication-setup.md)** - Set up X.509 certificate authentication
5. **[Monitoring & Diagnostics](monitoring-diagnostics.md)** - Set up comprehensive monitoring

### Production Deployment

For production environments, consider:

- **X.509 Certificate Authentication** for enhanced security
- **Automated Deployment** using configuration management tools
- **Monitoring and Alerting** for operational visibility
- **Update Testing** in staging environments
- **Rollback Procedures** for failed updates

### Development and Testing

For development scenarios:
- **[Building the Agent](how-to-build-agent-code.md)** - Development build options
- **[Extension Development](extension-development.md)** - Creating custom handlers
- **[SDK Integration](sdk-integration.md)** - Integrating with your applications
