# ISA-95 Nested Edge Architecture for Azure Device Update

> **⚠️ Disclaimer:** This document describes aspirational architecture. Some multi-tier nested edge scenarios described here are not yet fully supported in the current Azure Device Update agent or Azure IoT Edge runtime. Sections marked *[Aspirational]* indicate capabilities that are planned or under consideration.

## Overview

[ISA-95](https://www.isa.org/standards-and-publications/isa-standards/isa-standards-committees/isa95) (IEC 62443) defines a hierarchical model for industrial automation and control systems, organizing networks into discrete levels (0–4). This segmentation — often called the **Purdue Reference Model** — enforces strict boundaries between operational technology (OT) and information technology (IT) zones to minimize attack surface and contain failures.

Azure IoT Edge's **nested edge** capability allows IoT Edge devices to form parent–child hierarchies, bridging isolated network zones without granting lower-level devices direct internet access. Combined with **Azure Device Update (ADU)**, this architecture enables secure over-the-air update delivery across ISA-95 level boundaries.

This document maps ADU and IoT Edge concepts to the ISA-95 model and describes how multi-tier nested edge can support ISA-95-compliant deployments.

---

## ISA-95 Levels and Azure IoT Mapping

| ISA-95 Level | Description | Typical Systems | Azure IoT Mapping |
|---|---|---|---|
| **Level 4** | Enterprise / Cloud | ERP, cloud services | Azure IoT Hub, ADU Service |
| **Level 3.5** | DMZ | Firewalls, proxies | Top-level IoT Edge gateway |
| **Level 3** | Manufacturing Operations | MES, Historian | Mid-tier IoT Edge gateway *(optional)* |
| **Level 2** | Supervisory Control | SCADA, HMI | Lowest-tier IoT Edge gateway |
| **Level 0–1** | Process / Basic Control | PLCs, sensors, actuators | ADU leaf devices (agents) |

### Key Principles

- **No level skipping.** Devices at Level 0–1 communicate only with Level 2; Level 2 communicates only with Level 3 or 3.5. Traffic never traverses more than one boundary at a time.
- **Unidirectional preference.** Where possible, data flows upward (OT → IT). Commands flow downward through controlled channels.
- **Each boundary is enforced** by firewalls, VLANs, or physical network isolation.

---

## Architecture Diagram

```
┌──────────────────────────────────────────────────────┐
│  Level 4: Azure Cloud                                │
│    ┌──────────────────────────────┐                  │
│    │  Azure IoT Hub + ADU Service │                  │
│    └──────────────┬───────────────┘                  │
└───────────────────┼──────────────────────────────────┘
                    │  Internet / VPN
┌───────────────────┼──────────────────────────────────┐
│  Level 3.5: DMZ   │                                  │
│    ┌──────────────▼───────────────┐                  │
│    │  IoT Edge Gateway            │                  │
│    │  (Top-level parent)          │                  │
│    │  + Microsoft Connected Cache │                  │
│    └──────────────┬───────────────┘                  │
└───────────────────┼──────────────────────────────────┘
                    │  Firewalled (unidirectional preferred)
┌───────────────────┼──────────────────────────────────┐
│  Level 3: Site Operations (optional)                 │
│    ┌──────────────▼───────────────┐                  │
│    │  IoT Edge Gateway            │                  │
│    │  (Mid-tier)                  │                  │
│    │  + Microsoft Connected Cache │                  │
│    └──────────────┬───────────────┘                  │
└───────────────────┼──────────────────────────────────┘
                    │  Isolated OT network
┌───────────────────┼──────────────────────────────────┐
│  Level 2: Supervisory Control                        │
│    ┌──────────────▼───────────────┐                  │
│    │  IoT Edge Gateway            │                  │
│    │  (Lowest tier)               │                  │
│    │  + Microsoft Connected Cache │                  │
│    └──────────────┬───────────────┘                  │
└───────────────────┼──────────────────────────────────┘
                    │  Field bus / local LAN
┌───────────────────┼──────────────────────────────────┐
│  Level 0–1: Field Devices                            │
│    ┌──────────────▼───────────────┐                  │
│    │  ADU Agent (Leaf device)     │                  │
│    │  PLC / Sensor / Actuator     │                  │
│    └──────────────────────────────┘                  │
└──────────────────────────────────────────────────────┘
```

---

## How ADU Supports This Architecture

### Nested Edge Message Relay

Each IoT Edge layer acts as a transparent gateway, relaying device-to-cloud (D2C) and cloud-to-device (C2D) messages between its children and its parent. ADU leverages this chain for:

- **Deployment instructions** — ADU deployment actions flow from the cloud through each Edge tier down to the leaf device.
- **Device twin synchronization** — Reported and desired properties propagate through the nested gateway chain.
- **Telemetry and status** — Leaf devices report update status upward through each tier.

### Connection String Routing

Leaf devices and child Edge devices use the `GatewayHostName` property in their connection string to route through their immediate parent:

```
HostName=<iothub>.azure-devices.net;DeviceId=<device>;SharedAccessKey=<key>;GatewayHostName=<parent-edge-ip>
```

This ensures the device never attempts direct internet connectivity — all traffic is proxied through the parent Edge.

### Certificate Trust Chain

Each hop in the nested edge hierarchy requires trust establishment via `edgegatewayCertPath`:

```
Cloud root CA
  └─ DMZ Edge device CA
       └─ Mid-tier Edge device CA
            └─ Lowest-tier Edge device CA
                 └─ Leaf device identity cert
```

Each layer trusts only its immediate parent's certificate, enforcing the ISA-95 principle of no level skipping.

### Microsoft Connected Cache (MCC)

MCC modules deployed at each IoT Edge tier cache update content locally. This:

- **Reduces bandwidth** across level boundaries — content is downloaded once per tier.
- **Enables air-gapped delivery** — lower-level devices pull content from their local MCC rather than the internet.
- **Supports ISA-95 isolation** — each level's content comes from the level immediately above.

### Update Deployment Flow

```
ADU Service (Cloud)
  → Top-level Edge (DMZ)         Downloads content, caches in MCC
    → Mid-tier Edge (Level 3)    Pulls from DMZ MCC, caches locally
      → Lowest Edge (Level 2)    Pulls from Level 3 MCC, caches locally
        → Leaf device (Level 0–1) Pulls from Level 2 MCC, installs update
```

Device twin desired properties cascade downward; reported properties bubble upward through the same chain.

---

## Network Security Considerations

### Firewall and VLAN Rules

| Boundary | Direction | Allowed Traffic |
|---|---|---|
| Level 4 ↔ Level 3.5 | Outbound only (Edge → Hub) | AMQPS (443/5671), HTTPS (443) |
| Level 3.5 ↔ Level 3 | Bidirectional (constrained) | AMQPS, HTTPS on specific ports |
| Level 3 ↔ Level 2 | Bidirectional (constrained) | AMQPS, HTTPS on specific ports |
| Level 2 ↔ Level 0–1 | Bidirectional (constrained) | MQTT/AMQPS on local network only |

### Core Principles

- **Leaf devices NEVER have direct internet access.** All connectivity is mediated through the IoT Edge gateway hierarchy.
- **Only the top-level Edge device in the DMZ has outbound internet access**, and only to the required Azure endpoints.
- **Consider unidirectional gateways (data diodes)** at the Level 3.5 boundary for strict Purdue model compliance. This limits cloud-to-device communication but maximizes OT network protection.
- **Certificate-based authentication (X.509) is preferred** at lower levels to avoid shared access key management in constrained environments.
- **Network monitoring** should be deployed at each level boundary to detect anomalous cross-level traffic.

---

## Current Limitations & Roadmap

| Capability | Current State | Aspirational State *[Planned]* |
|---|---|---|
| **Nesting depth** | 1 level of nesting (leaf → parent Edge → IoT Hub) | Multi-hop nested edge (leaf → L2 Edge → L3 Edge → DMZ Edge → Hub) |
| **GatewayHostName** | Supports one hop in connection string | IoT Edge transparent gateway chaining for deep nesting |
| **Connected Cache** | MCC serves content one hop from the Edge device | Cascading MCC across multiple tiers for multi-level content delivery |
| **Deployment targeting** | Group-based targeting at leaf level | Hierarchical deployment policies respecting ISA-95 zone membership |
| **Certificate management** | Manual certificate provisioning per device | Automated certificate enrollment and renewal across nested tiers |

> **Note:** Check the [Azure IoT Edge nested edge documentation](https://learn.microsoft.com/azure/iot-edge/how-to-connect-downstream-iot-edge-device) for the latest supported nesting depth and configuration.

---

## Configuration Example *[Aspirational Multi-Tier]*

The following shows a conceptual configuration for a three-tier nested edge deployment mapped to ISA-95 levels.

### Top-Level Edge (DMZ — Level 3.5)

Connects directly to Azure IoT Hub.

```toml
# /etc/aziot/config.toml (Top-level Edge in DMZ)

[provisioning]
source = "manual"
connection_string = "HostName=contoso-hub.azure-devices.net;DeviceId=dmz-edge-01;SharedAccessKey=<key>"

# No GatewayHostName — this device has direct internet access

[edge_ca]
cert = "file:///etc/aziot/certs/dmz-edge-ca.pem"
pk = "file:///etc/aziot/keys/dmz-edge-ca.key"
```

### Mid-Tier Edge (Level 3 — Site Operations)

Routes through the DMZ Edge gateway.

```toml
# /etc/aziot/config.toml (Mid-tier Edge at Level 3)

[provisioning]
source = "manual"
connection_string = "HostName=contoso-hub.azure-devices.net;DeviceId=site-edge-01;SharedAccessKey=<key>;GatewayHostName=10.3.5.10"
#                                                                                                       ^^^^^^^^^^^^^^^^
#                                                                                       DMZ Edge IP (Level 3.5 network)

[edge_ca]
cert = "file:///etc/aziot/certs/site-edge-ca.pem"
pk = "file:///etc/aziot/keys/site-edge-ca.key"

# Trust the DMZ Edge's CA certificate
trust_bundle_cert = "file:///etc/aziot/certs/dmz-edge-ca.pem"
```

### Leaf Device (Level 0–1 — Field Device)

Routes through the lowest-tier Edge gateway.

```toml
# /etc/adu/du-config.json conceptual mapping
# Connection string uses GatewayHostName of the Level 2 Edge

# Device connection string:
# HostName=contoso-hub.azure-devices.net;DeviceId=plc-sensor-01;SharedAccessKey=<key>;GatewayHostName=10.2.0.10
#                                                                                                     ^^^^^^^^^
#                                                                                     Level 2 Edge IP (OT network)
```

```json
{
  "schemaVersion": "1.1",
  "aduShellTrustedUsers": ["adu", "do"],
  "iotHubProtocol": "mqtt",
  "manufacturer": "Contoso",
  "model": "PLC-Sensor-v1",
  "agents": [
    {
      "name": "main",
      "runas": "adu",
      "connectionSource": {
        "connectionType": "string",
        "connectionData": "HostName=contoso-hub.azure-devices.net;DeviceId=plc-sensor-01;SharedAccessKey=<key>;GatewayHostName=10.2.0.10"
      },
      "manufacturer": "Contoso",
      "model": "PLC-Sensor-v1"
    }
  ]
}
```

### Certificate Chain Summary

```
Cloud Root CA (Azure IoT Hub)
  └─ DMZ Edge CA         (trusted by: Mid-tier Edge, via trust_bundle_cert)
       └─ Site Edge CA    (trusted by: Level 2 Edge, via trust_bundle_cert)
            └─ L2 Edge CA (trusted by: Leaf device, via edgegatewayCertPath)
```

---

## Testing ISA-95 Compliance

### Local Test Environment

Refer to [nested-edge-test-environment.md](nested-edge-test-environment.md) for instructions on setting up a local test environment that simulates ISA-95 network segmentation.

### Simulating Level Boundaries with Docker Networks

Use separate Docker networks to represent each ISA-95 level and verify isolation:

```bash
# Create isolated networks for each ISA-95 level
docker network create --subnet=10.4.0.0/24 level4-cloud
docker network create --subnet=10.35.0.0/24 level35-dmz
docker network create --subnet=10.3.0.0/24 level3-site
docker network create --subnet=10.2.0.0/24 level2-supervisory
docker network create --subnet=10.1.0.0/24 level01-field

# Attach Edge containers to their respective level networks
# Each Edge container should have interfaces on exactly two adjacent levels
# Example: DMZ Edge has interfaces on level4-cloud AND level35-dmz
```

### Using VLANs for Hardware Test Beds

For physical test environments, assign each ISA-95 level to a separate VLAN:

| VLAN ID | ISA-95 Level | Subnet | Devices |
|---|---|---|---|
| 400 | Level 4 | 10.4.0.0/24 | Cloud connectivity endpoint |
| 350 | Level 3.5 (DMZ) | 10.35.0.0/24 | Top-level IoT Edge |
| 300 | Level 3 | 10.3.0.0/24 | Mid-tier IoT Edge |
| 200 | Level 2 | 10.2.0.0/24 | Lowest-tier IoT Edge |
| 100 | Level 0–1 | 10.1.0.0/24 | ADU leaf devices |

### Verification Checklist

- [ ] **No direct internet from Level 0–2** — From a leaf device or Level 2 Edge, confirm that `curl https://azure.com` times out.
- [ ] **Message relay works** — Deploy a test update from ADU and verify the leaf device receives it through the nested chain.
- [ ] **No cross-level traffic** — Use packet captures (e.g., `tcpdump`) at each boundary to verify that traffic only flows between adjacent levels.
- [ ] **MCC content caching** — Verify that update content is served from the local MCC at each level rather than being pulled from the internet.
- [ ] **Certificate validation** — Confirm that a device at Level 0–1 cannot connect directly to a Level 3 Edge (certificate mismatch should prevent it).
- [ ] **Twin propagation** — Verify that device twin reported properties from a leaf device are visible in IoT Hub, and desired property changes from IoT Hub reach the leaf device.

---

## References

- [ISA-95 / IEC 62443 Standards](https://www.isa.org/standards-and-publications/isa-standards/isa-standards-committees/isa95)
- [Azure IoT Edge Nested Edge](https://learn.microsoft.com/azure/iot-edge/how-to-connect-downstream-iot-edge-device)
- [Azure Device Update Documentation](https://learn.microsoft.com/azure/iot-hub-device-update/)
- [Microsoft Connected Cache](https://learn.microsoft.com/azure/iot-hub-device-update/connected-cache-overview)
- [IoT Edge Certificate Management](https://learn.microsoft.com/azure/iot-edge/how-to-manage-device-certificates)
