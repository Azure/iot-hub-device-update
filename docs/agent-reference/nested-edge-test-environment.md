# Nested Edge Test Environment

## Overview

This guide explains how to set up a local test environment using VMs or containers to simulate nested edge and gateway topologies for testing the Device Update for IoT Hub (ADU) agent. The goal is to validate the full communication path — leaf device → parent gateway → Azure IoT Hub — without requiring changes to cloud infrastructure.

This is useful for:

- Verifying that an ADU agent on a leaf device can receive deployments through an IoT Edge gateway.
- Testing content delivery via Microsoft Connected Cache (MCC) on the parent edge.
- Simulating network-constrained environments where leaf devices have no direct internet access.
- Regression testing gateway certificate handling and connection routing.

## Architecture

The test environment consists of two nodes (VMs or containers) and a cloud endpoint:

```
┌─────────────────────┐          ┌──────────────────────────┐          ┌──────────────────┐
│   Leaf Device VM    │          │   Parent Edge VM         │          │  Azure IoT Hub   │
│                     │          │                          │          │                  │
│   ADU Agent         │◄─LAN───►│   IoT Edge Runtime       │◄─Internet─►│  ADU Service     │
│                     │  only    │   MCC Module             │          │                  │
│   (no internet)     │          │   Edge Hub / Edge Agent  │          │                  │
└─────────────────────┘          └──────────────────────────┘          └──────────────────┘
```

- **Leaf Device VM** — Runs the ADU agent. Connected only to the parent edge device over a LAN segment. Has no direct internet access.
- **Parent Edge VM** — Runs the IoT Edge runtime configured as a transparent gateway. Has internet access to reach Azure IoT Hub. Hosts the MCC module for proxying content downloads.
- **Azure IoT Hub** — The cloud service with Device Update configured. The parent edge device and leaf device identities are registered here.

## Prerequisites

| Requirement | Details |
|---|---|
| 2 VMs or containers | One for the leaf device, one for the parent edge device. Ubuntu 20.04 or 22.04 recommended. |
| Azure IoT Hub | An IoT Hub instance with an IoT Edge device identity registered. |
| ADU agent | Built from source or installed via `.deb` package on the leaf device. |
| IoT Edge runtime | `aziot-edge` and `aziot-identity-service` packages for the parent device. |
| Certificates | A root CA and gateway server certificate for TLS between leaf and parent. |
| Network configuration | Ability to create isolated networks (iptables, Docker networks, or hypervisor internal networks). |

## Setup: Parent Edge Device (Gateway)

### 1. Install IoT Edge Runtime

```bash
# Add the Microsoft package repository
wget https://packages.microsoft.com/config/ubuntu/22.04/packages-microsoft-prod.deb -O packages-microsoft-prod.deb
sudo dpkg -i packages-microsoft-prod.deb
rm packages-microsoft-prod.deb

# Install IoT Edge
sudo apt-get update
sudo apt-get install -y aziot-edge
```

### 2. Configure as Transparent Gateway

Apply the IoT Edge configuration with the device connection string:

```bash
sudo cp /etc/aziot/config.toml.edge.template /etc/aziot/config.toml
```

Edit `/etc/aziot/config.toml` and set the provisioning section:

```toml
[provisioning]
source = "manual"
connection_string = "<PARENT_EDGE_DEVICE_CONNECTION_STRING>"
```

### 3. Generate and Install Gateway CA Certificates

Create a test CA and gateway server certificate. These are for development only — do not use in production.

```bash
# Clone the IoT Edge certificate helper scripts
git clone https://github.com/Azure/iotedge.git
cd iotedge/tools/CACertificates

# Generate root CA
./certGen.sh create_root_and_intermediate

# Generate the Edge device CA certificate signed by the root
./certGen.sh create_edge_device_ca_certificate "parent-edge-gw"
```

Configure the certificates in `/etc/aziot/config.toml`:

```toml
[edge_ca]
cert = "file:///var/aziot/certs/iot-edge-device-ca-parent-edge-gw-full-chain.cert.pem"
pk = "file:///var/aziot/secrets/iot-edge-device-ca-parent-edge-gw.key.pem"

[trust_bundle]
cert = "file:///var/aziot/certs/azure-iot-test-only.root.ca.cert.pem"
```

Copy the generated certificate and key files to the paths above:

```bash
sudo mkdir -p /var/aziot/certs /var/aziot/secrets

sudo cp certs/iot-edge-device-ca-parent-edge-gw-full-chain.cert.pem /var/aziot/certs/
sudo cp private/iot-edge-device-ca-parent-edge-gw.key.pem /var/aziot/secrets/
sudo cp certs/azure-iot-test-only.root.ca.cert.pem /var/aziot/certs/

sudo chown -R aziotcs:aziotcs /var/aziot/certs
sudo chown -R aziotks:aziotks /var/aziot/secrets
```

### 4. Expose MQTT/AMQPS on the Local Network

In `/etc/aziot/config.toml`, set the hostname to the parent VM's LAN IP or a resolvable hostname:

```toml
hostname = "parent-edge-gw"
```

Ensure that the Edge Hub module's port bindings include the standard IoT protocols. Edit the Edge Hub module's create options (via Azure portal or deployment manifest) to expose ports `8883` (MQTTS) and `5671` (AMQPS):

```json
{
  "HostConfig": {
    "PortBindings": {
      "8883/tcp": [{ "HostPort": "8883" }],
      "5671/tcp": [{ "HostPort": "5671" }],
      "443/tcp":  [{ "HostPort": "443" }]
    }
  }
}
```

### 5. Install Microsoft Connected Cache (MCC) Module

Add the MCC module to the Edge deployment manifest. This allows the parent to cache and serve content downloads to leaf devices.

In the Azure portal (or in a deployment JSON), add a module:

- **Name:** `MicrosoftConnectedCache`
- **Image URI:** `mcr.microsoft.com/connectedcache/linux/iot/mcc-ubuntu-iot-amd64:latest`
- **Container Create Options:**

```json
{
  "HostConfig": {
    "PortBindings": {
      "8081/tcp": [{ "HostPort": "80" }]
    }
  }
}
```

Set the module's environment variables as required by MCC (cache node ID, customer ID, etc.) per the [MCC documentation](https://learn.microsoft.com/azure/iot-hub-device-update/connected-cache-disconnected-device-update).

### 6. Register a Child Device Identity in IoT Hub

In Azure IoT Hub, create a new device identity for the leaf device and set its parent to the Edge device:

```bash
# Using the Azure CLI
az iot hub device-identity create \
  --hub-name <HUB_NAME> \
  --device-id leaf-device-01 \
  --auth-method shared_private_key

az iot hub device-identity parent set \
  --hub-name <HUB_NAME> \
  --device-id leaf-device-01 \
  --parent-device-id parent-edge-gw
```

### 7. Apply Configuration and Verify

```bash
sudo iotedge config apply

# Verify the runtime is running
sudo iotedge system status
sudo iotedge list
```

## Setup: Leaf Device (ADU Agent)

### 1. Install the ADU Agent

Install from a `.deb` package or from a local build:

```bash
# From .deb package
sudo apt-get install -y ./deviceupdate-agent_<version>_amd64.deb

# Or if built from source, copy the binary to /usr/bin
sudo cp out/bin/AducIotAgent /usr/bin/AducIotAgent
```

### 2. Configure du-config.json

Edit `/etc/adu/du-config.json` to point the agent at the parent gateway:

```json
{
  "schemaVersion": "1.1",
  "aduShellTrustedUsers": ["adu", "do"],
  "iotHubProtocol": "mqtt",
  "manufacturer": "test-manufacturer",
  "model": "test-model",
  "agents": [
    {
      "name": "main",
      "runas": "adu",
      "connectionSource": {
        "connectionType": "string",
        "connectionData": "HostName=<HUB_NAME>.azure-devices.net;DeviceId=leaf-device-01;SharedAccessKey=<KEY>;GatewayHostName=<PARENT_IP_OR_HOSTNAME>"
      },
      "manufacturer": "test-manufacturer",
      "model": "test-model"
    }
  ],
  "edgegatewayCertPath": "/etc/adu/certs/azure-iot-test-only.root.ca.cert.pem"
}
```

Key fields:

- **`GatewayHostName`** in the connection string — set to the parent edge VM's IP address or hostname (e.g., `parent-edge-gw`).
- **`edgegatewayCertPath`** — path to the root CA certificate used to sign the parent gateway's server certificate.

### 3. Install the Parent Gateway CA Certificate on the Leaf

Copy the root CA certificate from the parent edge VM to the leaf device:

```bash
# On the leaf device
sudo mkdir -p /etc/adu/certs
sudo scp user@<PARENT_IP>:/var/aziot/certs/azure-iot-test-only.root.ca.cert.pem /etc/adu/certs/

# Also install into the system trust store so all TLS clients trust it
sudo cp /etc/adu/certs/azure-iot-test-only.root.ca.cert.pem /usr/local/share/ca-certificates/azure-iot-test-only.root.ca.cert.crt
sudo update-ca-certificates
```

### 4. Configure DNS / Hosts Resolution

The leaf device must resolve the `GatewayHostName` to the parent's LAN IP. If there is no DNS server on the isolated network, add a hosts entry:

```bash
echo "<PARENT_LAN_IP>  parent-edge-gw" | sudo tee -a /etc/hosts
```

### 5. For X.509 Authentication (Optional)

If using X.509 certificate-based authentication instead of symmetric keys:

```bash
# Generate a leaf device client certificate signed by the same root CA
cd iotedge/tools/CACertificates
./certGen.sh create_device_certificate "leaf-device-01"

# Copy to the leaf device
sudo mkdir -p /etc/adu/certs
sudo cp certs/iot-device-leaf-device-01-full-chain.cert.pem /etc/adu/certs/
sudo cp private/iot-device-leaf-device-01.key.pem /etc/adu/certs/
```

Update the connection source in `du-config.json` to use X.509:

```json
"connectionSource": {
  "connectionType": "string",
  "connectionData": "HostName=<HUB_NAME>.azure-devices.net;DeviceId=leaf-device-01;x509=true;GatewayHostName=<PARENT_IP_OR_HOSTNAME>"
}
```

And configure the identity service or agent to reference the client certificate and key.

## Network Isolation

To accurately simulate a nested edge topology, the leaf device should have **no direct internet access**. All cloud communication must flow through the parent gateway.

### Using iptables / nftables

On the leaf VM, block all outbound traffic except to the parent edge device:

```bash
PARENT_IP="192.168.1.10"

# Flush existing rules
sudo iptables -F OUTPUT

# Allow traffic to the parent gateway
sudo iptables -A OUTPUT -d $PARENT_IP -j ACCEPT

# Allow loopback
sudo iptables -A OUTPUT -o lo -j ACCEPT

# Allow established/related connections (responses)
sudo iptables -A OUTPUT -m state --state ESTABLISHED,RELATED -j ACCEPT

# Block everything else outbound
sudo iptables -A OUTPUT -j DROP
```

### Using Docker Networks

Create an internal Docker network with no external connectivity:

```bash
# Create an isolated network (no internet access)
docker network create --internal --subnet=172.28.0.0/16 edge-test-net
```

Containers on this network cannot reach the internet. The parent container should also be attached to a second network that does have internet access.

### Using Hyper-V / VirtualBox Internal Networks

| Hypervisor | Configuration |
|---|---|
| **VirtualBox** | Set the leaf VM's network adapter to "Internal Network". Set the parent VM to have two adapters: one "Internal Network" (shared with leaf) and one "NAT" or "Bridged" (for internet). |
| **Hyper-V** | Create an internal virtual switch. Attach the leaf VM only to the internal switch. Attach the parent VM to both the internal switch and an external switch. |

### Verify Isolation

From the leaf device, confirm it can reach the parent but **not** the internet:

```bash
# Should succeed
ping -c 3 <PARENT_IP>
openssl s_client -connect <PARENT_IP>:8883 -CAfile /etc/adu/certs/azure-iot-test-only.root.ca.cert.pem

# Should fail (timeout or unreachable)
ping -c 3 8.8.8.8
curl -v https://www.microsoft.com
```

## Testing Scenarios

### 1. Leaf Device Connects and Reports to IoT Hub via Gateway

Start the ADU agent on the leaf device and verify it successfully connects through the parent gateway:

```bash
sudo systemctl start deviceupdate-agent

# Check agent logs for a successful connection
sudo journalctl -u deviceupdate-agent -f --no-pager | head -50
```

Look for log lines indicating a successful MQTT connection and twin reported properties being sent.

### 2. ADU Deployment Reaches Leaf Device Through Parent

1. In the Azure portal, create an update deployment targeting the leaf device's device group.
2. Monitor the agent logs on the leaf device for the incoming deployment action.
3. Verify the agent transitions through the expected states: idle → download → install → apply → idle.

### 3. Content Download Through MCC on Parent

When the leaf device downloads update content, verify the download is served through the MCC module on the parent edge:

```bash
# On the parent edge, check MCC module logs
sudo iotedge logs MicrosoftConnectedCache --tail 50
```

Look for cache hit/miss entries corresponding to the content requested by the leaf device.

### 4. Leaf Device Twin Updates Flow Correctly

Use the Azure CLI or portal to read the leaf device's module twin and confirm that:

- Reported properties are updated by the ADU agent.
- Desired properties set by the ADU service are received by the agent.

```bash
az iot hub device-twin show --hub-name <HUB_NAME> --device-id leaf-device-01 \
  | jq '.properties.reported.deviceUpdate'
```

### 5. Network Partition and Recovery Testing

Simulate a network interruption between the leaf and parent:

```bash
# On the leaf VM — block traffic to the parent temporarily
sudo iptables -I OUTPUT 1 -d <PARENT_IP> -j DROP

# Wait for the agent to detect disconnection (check logs)
sleep 60

# Restore connectivity
sudo iptables -D OUTPUT -d <PARENT_IP> -j DROP
```

Verify the ADU agent reconnects and resumes any in-progress operations (download retry, state re-sync).

## Docker Compose Example

The following `docker-compose.yml` provides a simplified two-container setup for quick local testing. This is **not** a production configuration.

```yaml
# docker-compose.yml — Nested Edge Test Environment
#
# This creates:
#   - An isolated network with no external access (for the leaf device)
#   - A bridged network with external access (for the parent gateway)
#   - A parent container running IoT Edge + MCC
#   - A leaf container running the ADU agent

version: "3.8"

networks:
  # Internal-only network connecting leaf to parent (no internet)
  edge-internal:
    driver: bridge
    internal: true
    ipam:
      config:
        - subnet: 172.28.0.0/24

  # External network for the parent gateway to reach Azure
  edge-external:
    driver: bridge

services:
  parent-edge:
    image: ubuntu:22.04
    hostname: parent-edge-gw
    container_name: parent-edge-gw
    privileged: true
    networks:
      edge-internal:
        ipv4_address: 172.28.0.10
      edge-external: {}
    volumes:
      - ./certs:/var/aziot/certs:ro
      - ./secrets:/var/aziot/secrets:ro
      - ./config/parent-config.toml:/etc/aziot/config.toml:ro
    ports:
      - "8883:8883"
      - "5671:5671"
      - "443:443"
    entrypoint: >
      bash -c "
        apt-get update && apt-get install -y curl &&
        curl -sSL https://packages.microsoft.com/config/ubuntu/22.04/packages-microsoft-prod.deb -o packages-microsoft-prod.deb &&
        dpkg -i packages-microsoft-prod.deb &&
        apt-get update && apt-get install -y aziot-edge &&
        iotedge config apply &&
        tail -f /dev/null
      "

  leaf-device:
    image: ubuntu:22.04
    hostname: leaf-device-01
    container_name: leaf-device-01
    depends_on:
      - parent-edge
    networks:
      edge-internal:
        ipv4_address: 172.28.0.20
      # NOTE: no edge-external — leaf has no internet access
    volumes:
      - ./certs/azure-iot-test-only.root.ca.cert.pem:/etc/adu/certs/azure-iot-test-only.root.ca.cert.pem:ro
      - ./config/du-config.json:/etc/adu/du-config.json:ro
      - ./packages/deviceupdate-agent.deb:/opt/deviceupdate-agent.deb:ro
    extra_hosts:
      - "parent-edge-gw:172.28.0.10"
    entrypoint: >
      bash -c "
        apt-get update &&
        dpkg -i /opt/deviceupdate-agent.deb || apt-get install -f -y &&
        cp /etc/adu/certs/azure-iot-test-only.root.ca.cert.pem /usr/local/share/ca-certificates/azure-iot-test-only.crt &&
        update-ca-certificates &&
        AducIotAgent -l 2 --config-folder /etc/adu
      "
```

Before running, prepare the required files:

```
.
├── docker-compose.yml
├── certs/
│   ├── azure-iot-test-only.root.ca.cert.pem
│   ├── iot-edge-device-ca-parent-edge-gw-full-chain.cert.pem
│   └── ...
├── secrets/
│   └── iot-edge-device-ca-parent-edge-gw.key.pem
├── config/
│   ├── parent-config.toml    # IoT Edge config for parent
│   └── du-config.json        # ADU agent config for leaf
└── packages/
    └── deviceupdate-agent.deb
```

Start the environment:

```bash
docker compose up --build
```

> **Note:** The parent container's `entrypoint` installs IoT Edge at startup for simplicity. For faster iteration, build a custom image with IoT Edge pre-installed.

## Troubleshooting

### Gateway TLS Validation Failures

**Symptom:** The ADU agent logs show TLS handshake errors or certificate verification failures when connecting to the parent gateway.

**Checks:**

```bash
# Verify the root CA cert is correctly installed on the leaf
openssl verify -CAfile /etc/adu/certs/azure-iot-test-only.root.ca.cert.pem \
  <(echo | openssl s_client -connect parent-edge-gw:8883 2>/dev/null | openssl x509)

# Confirm the certificate chain is complete on the parent
openssl x509 -in /var/aziot/certs/iot-edge-device-ca-parent-edge-gw-full-chain.cert.pem -noout -text | grep -A1 "Issuer"

# Ensure edgegatewayCertPath in du-config.json points to the correct file
cat /etc/adu/du-config.json | grep edgegatewayCertPath
```

- Confirm that the `hostname` in the parent's `config.toml` matches the `GatewayHostName` in the leaf's connection string.
- Ensure the certificate's Subject or SAN includes the gateway hostname.

### MQTT Connection Refused

**Symptom:** The agent cannot establish an MQTT connection to the parent on port 8883.

**Checks:**

```bash
# From the leaf device — test TCP connectivity
nc -zv parent-edge-gw 8883

# On the parent — verify Edge Hub is running and listening
sudo iotedge list
sudo ss -tlnp | grep 8883
```

- Ensure the Edge Hub module has port 8883 in its `HostConfig.PortBindings`.
- If using Docker, make sure no firewall rules are blocking inter-container traffic on the internal network.

### Content Download Timeouts

**Symptom:** The ADU agent begins a download but it stalls or times out.

**Checks:**

```bash
# Verify MCC module is running on the parent
sudo iotedge logs MicrosoftConnectedCache --tail 20

# Test HTTP connectivity from leaf to parent on port 80 (MCC)
curl -v http://parent-edge-gw:80
```

- The leaf's Delivery Optimization client must be configured to use the parent edge as its cache server.
- Check that the MCC module's environment variables (customer ID, cache node ID) are set correctly.

### Certificate Chain Issues

**Symptom:** `X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY` or similar OpenSSL errors.

**Checks:**

```bash
# View the full chain served by the parent gateway
openssl s_client -showcerts -connect parent-edge-gw:8883 < /dev/null 2>/dev/null

# Ensure the root CA is in the system trust store on the leaf
ls /usr/local/share/ca-certificates/ | grep azure-iot
update-ca-certificates --fresh
```

- If using `certGen.sh`, make sure you use the `-full-chain` certificate file that includes intermediate certificates.
- If regenerating certificates, restart the IoT Edge runtime on the parent (`sudo iotedge config apply`) and restart the ADU agent on the leaf.
