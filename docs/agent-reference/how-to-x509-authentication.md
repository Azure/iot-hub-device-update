# How to Use X.509 Client Certificate Authentication

This document describes how to configure, test, and troubleshoot X.509 client certificate authentication for the Azure Device Update (ADU) agent, based on comprehensive testing and validation.

## Overview

X.509 client certificate authentication provides a more secure alternative to SAS token authentication by using certificates for device authentication with Azure IoT Hub. This method is particularly useful in enterprise scenarios that require certificate-based authentication and supports both device and module identities.

## Features

- **Client Certificate Authentication**: Authenticate devices using X.509 client certificates with proper extensions
- **CA Certificate Support**: Configure CA certificates for trust chain validation
- **PKCS#11 Integration**: Support for Hardware Security Modules (HSMs) via PKCS#11
- **OpenSSL Engine Support**: Flexible private key storage options
- **Device Twin Communication**: Full support for Azure IoT Hub Device Twin operations
- **Comprehensive Testing**: Complete validation framework for certificate functionality

## Prerequisites

### System Requirements
- OpenSSL 1.1.1 or later
- CA certificates package (for validating Azure IoT Hub server certificates)
- Appropriate network connectivity (port 8883 for MQTT, port 443 for HTTPS)

### Azure IoT Hub Setup
- Device registered in Azure IoT Hub
- X.509 authentication method configured
- Certificate thumbprint registered in device identity

## Configuration

### Basic X.509 Configuration

Configure the ADU agent to use X.509 authentication by updating your `du-config.json`:

```json
{
    "schemaVersion": "1.1",
    "aduShellTrustedUsers": ["adu", "do"],
    "manufacturer": "device_info_manufacturer",
    "model": "device_info_model",
    "agents": [
        {
            "name": "host-update",
            "runas": "adu",
            "connectionSource": {
                "connectionType": "X509",
                "connectionData": "HostName=your-hub.azure-devices.net;DeviceId=your-device-id;x509=true",
                "connectionX509CertFilePath": "/etc/adu/certs/client.pem",
                "connectionX509PrivateKeyFilePath": "/etc/adu/certs/client.key",
                "connectionX509CaCertFilePath": "/etc/adu/certs/ca.pem"
            },
            "manufacturer": "Contoso",
            "model": "Smart-Box"
        }
    ]
}
```

### Module Identity X.509 Configuration

For IoT Edge scenarios where the ADU agent runs as a module, configure module identity with X.509:

```json
{
    "schemaVersion": "1.1",
    "aduShellTrustedUsers": ["adu", "do"],
    "manufacturer": "device_info_manufacturer",
    "model": "device_info_model",
    "agents": [
        {
            "name": "host-update",
            "runas": "adu",
            "connectionSource": {
                "connectionType": "X509",
                "connectionData": "HostName=your-hub.azure-devices.net;DeviceId=your-device-id;ModuleId=your-module-id;x509=true",
                "connectionX509CertFilePath": "/etc/adu/certs/module-client.pem",
                "connectionX509PrivateKeyFilePath": "/etc/adu/certs/module-client.key",
                "connectionX509CaCertFilePath": "/etc/adu/certs/ca.pem"
            },
            "manufacturer": "Contoso",
            "model": "Smart-Box"
        }
    ]
}
```

### PKCS#11 Configuration

For Hardware Security Module support, configure PKCS#11:

```json
{
    "connectionSource": {
        "connectionType": "X509",
        "connectionData": "HostName=your-hub.azure-devices.net;DeviceId=your-device-id;x509=true",
        "connectionX509CertFilePath": "/etc/adu/certs/client.pem",
        "connectionX509PrivateKeyFilePath": "pkcs11:token=MyToken;object=MyKey",
        "opensslEngine": "pkcs11",
        "connectionX509CaCertFilePath": "/etc/adu/certs/ca.pem"
    }
}
```

### Configuration Parameters

| Parameter | Description | Required |
|-----------|-------------|----------|
| `connectionType` | Must be set to `"X509"` | Yes |
| `connectionData` | IoT Hub connection string with `x509=true`. For module identity, include `ModuleId=your-module-id` | Yes |
| `connectionX509CertFilePath` | Path to client certificate file (PEM format) | Yes |
| `connectionX509PrivateKeyFilePath` | Path to private key file or PKCS#11 URI | Yes |
| `connectionX509CaCertFilePath` | Path to CA certificate file (PEM format) | Yes |
| `opensslEngine` | OpenSSL engine identifier (e.g., "pkcs11") | Optional |

**Connection String Format:**
- **Device Identity**: `"HostName=hub.azure-devices.net;DeviceId=deviceId;x509=true"`
- **Module Identity**: `"HostName=hub.azure-devices.net;DeviceId=deviceId;ModuleId=moduleId;x509=true"`

## Enhanced Certificate Generation

### Generate Production-Ready Test Certificates

For comprehensive testing with proper IoT device extensions:

```bash
#!/bin/bash
# Enhanced certificate generation with proper X.509v3 extensions

CERT_DIR="./adu-certs"
DEVICE_ID="your-device-id"

# Create certificate directory
mkdir -p "$CERT_DIR"

echo "Generating enhanced certificates for IoT device authentication..."

# Generate CA private key
openssl genrsa -out "$CERT_DIR/ca.key" 2048

# Generate CA certificate with proper extensions (valid for 10 years)
openssl req -new -x509 -days 3650 -key "$CERT_DIR/ca.key" -out "$CERT_DIR/ca.pem" \
    -subj "/C=US/ST=WA/O=Contoso/CN=Contoso-CA" \
    -extensions v3_ca \
    -config <(echo '[req]'; echo 'distinguished_name=req'; echo '[v3_ca]'; echo 'basicConstraints=CA:TRUE'; echo 'keyUsage=keyCertSign,cRLSign'; echo 'subjectKeyIdentifier=hash'; echo 'authorityKeyIdentifier=keyid:always,issuer:always')

# Generate client private key
openssl genrsa -out "$CERT_DIR/client.key" 2048

# Generate client certificate signing request
openssl req -new -key "$CERT_DIR/client.key" -out "$CERT_DIR/client.csr" \
    -subj "/C=US/ST=WA/O=Contoso/CN=$DEVICE_ID"

# Generate client certificate with IoT device extensions (valid for 1 year)
openssl x509 -req -days 365 -in "$CERT_DIR/client.csr" \
    -CA "$CERT_DIR/ca.pem" -CAkey "$CERT_DIR/ca.key" -CAcreateserial \
    -out "$CERT_DIR/client.pem" \
    -extensions v3_client \
    -extfile <(echo '[v3_client]'; echo 'basicConstraints=CA:FALSE'; echo 'keyUsage=digitalSignature,keyEncipherment'; echo 'extendedKeyUsage=clientAuth'; echo 'subjectKeyIdentifier=hash'; echo 'authorityKeyIdentifier=keyid,issuer')

# Clean up CSR file
rm "$CERT_DIR/client.csr"

echo "Enhanced certificates generated successfully!"

# Display certificate information
echo ""
echo "Certificate Information:"
openssl x509 -in "$CERT_DIR/client.pem" -noout -subject -dates
echo ""
echo "SHA1 Thumbprint (for IoT Hub registration):"
openssl x509 -in "$CERT_DIR/client.pem" -noout -sha1 -fingerprint | sed 's/[:]//g' | sed 's/SHA1 Fingerprint=//'
```

### Certificate Extension Validation

Verify your certificates have the correct extensions for IoT device authentication:

```bash
# Check certificate extensions
openssl x509 -in client.pem -noout -text | grep -A 10 "X509v3 extensions"

# Required extensions for IoT devices:
# - Basic Constraints: CA:FALSE
# - Key Usage: Digital Signature, Key Encipherment  
# - Extended Key Usage: TLS Web Client Authentication
# - Subject Key Identifier and Authority Key Identifier
```

### Generate Module Certificates

For IoT Edge module scenarios, generate module-specific certificates:

```bash
# Generate module private key
openssl genrsa -out module-client.key 2048

# Generate module certificate request
openssl req -new -key module-client.key \
    -subj "/C=US/ST=WA/O=Contoso/CN=your-device-id/your-module-id" \
    -out module-client.csr

# Generate module certificate signed by CA with proper extensions
openssl x509 -req -days 365 -in module-client.csr \
    -CA ca.pem -CAkey ca.key -CAcreateserial \
    -out module-client.pem \
    -extensions v3_client \
    -extfile <(echo '[v3_client]'; echo 'basicConstraints=CA:FALSE'; echo 'keyUsage=digitalSignature,keyEncipherment'; echo 'extendedKeyUsage=clientAuth'; echo 'subjectKeyIdentifier=hash'; echo 'authorityKeyIdentifier=keyid,issuer')

# Clean up
rm module-client.csr
```

### Certificate Installation

```bash
# Create certificate directory
sudo mkdir -p /etc/adu/certs

# Install certificates with proper permissions
sudo cp client.pem /etc/adu/certs/
sudo cp client.key /etc/adu/certs/
sudo cp ca.pem /etc/adu/certs/

# Set appropriate permissions
sudo chmod 644 /etc/adu/certs/client.pem
sudo chmod 644 /etc/adu/certs/ca.pem
sudo chmod 600 /etc/adu/certs/client.key
sudo chown -R adu:adu /etc/adu/certs/

# Verify installation
echo "Certificate installation verification:"
sudo ls -la /etc/adu/certs/
```

## Comprehensive Testing Framework

### Automated Demo Script

Use the comprehensive demo script located in `docs/agent-reference/x509-demo/demo-setup.sh`:

```bash
# Run complete X.509 test with device registration and IoT Hub connectivity
./demo-setup.sh --device-id your-device-id --test-connection your-hub.azure-devices.net

# The script performs 8 comprehensive verification steps:
# 1. Certificate File Integrity Checks
# 2. Certificate-Key Pair Validation  
# 3. Certificate Chain Validation
# 4. Certificate Details Verification
# 5. Certificate Thumbprints Generation
# 6. TLS Connection Tests
# 7. MQTT Protocol Tests
# 8. Connection Summary with troubleshooting guidance
```

### Device Twin Communication Testing

Test Device Twin specific functionality:

```bash
# Use the device twin test script
./test-device-twin.sh your-device-id your-hub.azure-devices.net

# Tests:
# - Device Twin GET requests
# - Device Twin PATCH requests  
# - HTTPS connection validation
# - Certificate requirements validation
```

### Manual Testing Steps

#### 1. Certificate Validation

```bash
# Verify certificate format and validity
openssl x509 -in /etc/adu/certs/client.pem -text -noout
openssl rsa -in /etc/adu/certs/client.key -check -noout

# Verify certificate chain
openssl verify -CAfile /etc/adu/certs/ca.pem /etc/adu/certs/client.pem

# Check certificate-key pair matching
cert_modulus=$(openssl x509 -in /etc/adu/certs/client.pem -noout -modulus)
key_modulus=$(openssl rsa -in /etc/adu/certs/client.key -noout -modulus)
[ "$cert_modulus" = "$key_modulus" ] && echo "✅ Certificate and key match" || echo "❌ Certificate and key mismatch"
```

#### 2. System Root Certificate Verification

```bash
# Verify system can validate Azure IoT Hub certificates
openssl s_client -connect your-hub.azure-devices.net:8883 -CApath /etc/ssl/certs -verify_return_error -quiet < /dev/null

# Update CA certificates if needed
sudo apt update && sudo apt install ca-certificates
```

#### 3. TLS/HTTPS Connection Testing

```bash
# Test TLS connection with client certificate
openssl s_client -connect your-hub.azure-devices.net:8883 \
    -cert /etc/adu/certs/client.pem \
    -key /etc/adu/certs/client.key \
    -servername your-hub.azure-devices.net \
    -verify_return_error

# Test HTTPS connection for Device Twin API
curl -v --cert /etc/adu/certs/client.pem \
    --key /etc/adu/certs/client.key \
    --cacert /etc/ssl/certs/ca-certificates.crt \
    "https://your-hub.azure-devices.net/twins/your-device-id?api-version=2020-03-13"
```

#### 4. MQTT Protocol Testing

```bash
# Install mosquitto client if not available
sudo apt install mosquitto-clients

# Test MQTT connection with X.509 authentication
timeout 10s mosquitto_pub -h your-hub.azure-devices.net -p 8883 \
    -i your-device-id \
    -t "devices/your-device-id/messages/events/" \
    -m "test message" \
    --cert /etc/adu/certs/client.pem \
    --key /etc/adu/certs/client.key \
    --capath /etc/ssl/certs -d

# Expected: Connection successful, possible authorization rejection (normal for unregistered devices)
```

#### 5. Unit Testing

Run the existing unit tests to verify X.509 functionality:

```bash
# Build tests
cd /path/to/adu-agent
./scripts/build.sh --build-unit-tests

# Run configuration tests
./build/src/utils/config_utils/tests/config_utils_ut

# Run communication manager tests
./build/src/communication_managers/iothub_communication_manager/tests/iothub_communication_manager_ut

# Run EIS utility tests  
./build/src/utils/eis_utils/tests/eis_utils_ut
```

#### 6. Agent Integration Testing

Test the complete ADU agent with X.509 authentication:

```bash
# Install configuration
sudo cp your-du-config.json /etc/adu/du-config.json

# Restart the agent
sudo systemctl restart deviceupdate-agent.service

# Monitor logs for successful connection
sudo journalctl -u deviceupdate-agent.service -f

# Check authentication success
sudo journalctl -u deviceupdate-agent.service --since "5 minutes ago" | grep -i "connected\|authenticated"
```

### PKCS#11 Testing

For testing PKCS#11 functionality with SoftHSM:

```bash
# Install SoftHSM
sudo apt-get install softhsm2

# Initialize token
softhsm2-util --init-token --slot 0 --label "TestToken" --pin 1234 --so-pin 5678

# Import private key
softhsm2-util --import client.key --slot 0 --label "TestKey" --id 0001 --pin 1234

# List tokens
softhsm2-util --show-slots

# Set environment variable
export PKCS11_MODULE_PATH="/usr/lib/x86_64-linux-gnu/softhsm/libsofthsm2.so"

# Update configuration to use PKCS#11 URI
# "connectionX509PrivateKeyFilePath": "pkcs11:token=TestToken;object=TestKey;pin-value=1234"

# Test PKCS#11 functionality
openssl engine pkcs11 -t
```

## Azure IoT Hub Device Registration

### Register Device with X.509 Authentication

#### Using Azure Portal:
1. Navigate to your IoT Hub → Device management → Devices
2. Click **+ Add Device**
3. Enter Device ID (must match certificate Common Name)
4. Set Authentication type to **X.509 Self-Signed**
5. Set Primary Thumbprint (SHA1 fingerprint from your certificate)
6. Optionally set Secondary Thumbprint
7. Click **Save**

#### Using Azure CLI:
```bash
# Get certificate thumbprint
THUMBPRINT=$(openssl x509 -in client.pem -noout -sha1 -fingerprint | sed 's/[:]//g' | sed 's/SHA1 Fingerprint=//')

# Create device
az iot hub device-identity create \
  --hub-name your-hub-name \
  --device-id your-device-id \
  --auth-method x509_thumbprint \
  --primary-thumbprint $THUMBPRINT

# Verify device creation
az iot hub device-identity show \
  --hub-name your-hub-name \
  --device-id your-device-id
```

#### Getting Certificate Thumbprint:
```bash
# SHA1 thumbprint (for primary/secondary thumbprint field)
openssl x509 -in client.pem -noout -sha1 -fingerprint | sed 's/[:]//g' | sed 's/SHA1 Fingerprint=//'

# SHA256 thumbprint (alternative)
openssl x509 -in client.pem -noout -sha256 -fingerprint | sed 's/[:]//g' | sed 's/SHA256 Fingerprint=//'
```

## Troubleshooting

### Certificate Authentication Issues

#### Common Problems and Solutions:

**1. Certificate Permission Errors**
```bash
# Fix certificate permissions
sudo chmod 644 /etc/adu/certs/*.pem
sudo chmod 600 /etc/adu/certs/*.key
sudo chown -R adu:adu /etc/adu/certs/

# Verify permissions
ls -la /etc/adu/certs/
```

**2. Certificate Chain Validation Failures**
```bash
# Verify certificate chain
openssl verify -CAfile /etc/adu/certs/ca.pem /etc/adu/certs/client.pem

# Check certificate details
openssl x509 -in /etc/adu/certs/client.pem -text -noout

# Verify certificate extensions
openssl x509 -in /etc/adu/certs/client.pem -noout -text | grep -A 20 "X509v3 extensions"
```

**3. Certificate-Key Mismatch**
```bash
# Verify certificate and key match
cert_modulus=$(openssl x509 -in /etc/adu/certs/client.pem -noout -modulus)
key_modulus=$(openssl rsa -in /etc/adu/certs/client.key -noout -modulus)
if [ "$cert_modulus" = "$key_modulus" ]; then
    echo "✅ Certificate and key match"
else
    echo "❌ Certificate and key do not match - regenerate certificates"
fi
```

**4. Device Twin Communication Issues**

**Problem**: Device Twin operations fail with authentication errors
**Cause**: Missing or incorrect certificate extensions
**Solution**:
```bash
# Regenerate certificates with proper IoT device extensions
# Use the enhanced certificate generation script above
# Ensure certificate has:
# - basicConstraints=CA:FALSE
# - keyUsage=digitalSignature,keyEncipherment
# - extendedKeyUsage=clientAuth
```

**5. PKCS#11 Module Issues**
```bash
# Check SoftHSM installation
dpkg -l | grep softhsm

# Verify module path
ls -la /usr/lib/x86_64-linux-gnu/softhsm/libsofthsm2.so

# Set environment variable
export PKCS11_MODULE_PATH="/usr/lib/x86_64-linux-gnu/softhsm/libsofthsm2.so"

# Test PKCS#11 functionality
openssl engine pkcs11 -t
```

**6. Connection Timeout Issues**
```bash
# Check network connectivity
ping your-hub.azure-devices.net

# Test port accessibility
nc -zv your-hub.azure-devices.net 8883
nc -zv your-hub.azure-devices.net 443

# Check firewall rules
sudo ufw status
```

**7. Certificate Expiration**
```bash
# Check certificate validity
openssl x509 -in /etc/adu/certs/client.pem -noout -dates

# Check if certificate is currently valid
openssl x509 -in /etc/adu/certs/client.pem -checkend 0
```

### Advanced Troubleshooting

#### Log Analysis

Monitor agent logs for X.509-related messages:

```bash
# Follow agent logs in real-time
sudo journalctl -u deviceupdate-agent.service -f

# Search for authentication-related logs
sudo journalctl -u deviceupdate-agent.service | grep -i "x509\|cert\|auth\|tls"

# Check for connection errors
sudo journalctl -u deviceupdate-agent.service | grep -i "error\|fail\|denied"

# View recent startup logs
sudo journalctl -u deviceupdate-agent.service --since "10 minutes ago"
```

#### Network Analysis

```bash
# Capture TLS handshake with tcpdump
sudo tcpdump -i any -w x509-capture.pcap host your-hub.azure-devices.net and port 8883

# Test with verbose OpenSSL output
openssl s_client -connect your-hub.azure-devices.net:8883 \
    -cert /etc/adu/certs/client.pem \
    -key /etc/adu/certs/client.key \
    -debug -msg -state
```

#### Configuration Debugging

```bash
# Validate JSON configuration syntax
sudo python3 -m json.tool /etc/adu/du-config.json

# Check file paths in configuration
sudo grep -E "connectionX509.*FilePath" /etc/adu/du-config.json

# Verify all certificate files exist and are readable by adu user
sudo -u adu ls -la /etc/adu/certs/
```

### Known Issues and Workarounds

**Issue**: MQTT connection receives "Connection Refused: not authorized" even with registered device
**Cause**: Device may be registered but lacks necessary IoT Hub permissions
**Workaround**: Verify device registration includes correct thumbprint and authentication method

**Issue**: Device Twin API calls return 401 Unauthorized
**Cause**: Certificate authentication succeeds but authorization fails
**Solution**: Ensure device is properly registered in IoT Hub with correct X.509 thumbprint

**Issue**: TLS handshake fails with certificate verification errors
**Cause**: System lacks Azure IoT Hub root certificates
**Solution**: Update ca-certificates package: `sudo apt update && sudo apt install ca-certificates`

## Performance Considerations

- **Certificate Parsing**: Performed once during agent startup - no runtime impact
- **PKCS#11 Operations**: May have 10-50ms higher latency than file-based keys
- **Certificate File Size**: Large certificate files (>4KB) may impact startup time by 100-500ms
- **Memory Usage**: X.509 authentication adds approximately 2-4MB to agent memory footprint

## Security Best Practices

### Certificate Management
- **Private Key Protection**: Store private keys with 600 permissions, owned by adu user
- **Certificate Rotation**: Implement automated certificate rotation before expiration
- **Certificate Backup**: Securely backup CA private keys and certificates
- **Access Control**: Limit certificate file access to the minimum required users

### Production Deployment
- **Hardware Security**: Use PKCS#11 and HSMs for production private key storage
- **Certificate Validation**: Validate certificate extensions and key usage before deployment
- **Monitoring**: Implement certificate expiration monitoring and alerting
- **Audit Logging**: Enable certificate usage audit logging

### Network Security
- **TLS Configuration**: Ensure TLS 1.2 or higher is enforced
- **Certificate Pinning**: Consider certificate pinning for additional security
- **Network Segmentation**: Isolate IoT devices on separate network segments

## Related Documentation

- [How to Build Agent Code](how-to-build-agent-code.md)
- [How to Run Agent](how-to-run-agent.md) 
- [Device Update Agent Extensibility Points](device-update-agent-extensibility-points.md)
- [Troubleshooting Guide](how-to-troubleshoot-guide.md)

## Demo Scripts

Complete testing scripts are available in the `x509-demo` directory:

- **`demo-setup.sh`**: Comprehensive X.509 setup and testing script
- **`test-device-twin.sh`**: Device Twin communication testing script

These scripts provide production-ready certificate generation, installation, and validation workflows for X.509 authentication testing.
