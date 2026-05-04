# ADU Leaf Device Update Testing

This guide walks through creating a test environment for verifying that a leaf device can receive and apply updates from the Azure Device Update (ADU) service through a gateway device. It covers the full end-to-end flow: importing an update to the ADU service, deploying it to a device group, having the leaf device download content via the gateway, applying the update, and reporting success.

> **Audience:** Developers testing ADU functionality in nested IoT Edge scenarios.

## Prerequisites

Before you begin, make sure the following are in place:

- **Azure subscription** with an IoT Hub and an ADU instance provisioned.
- **IoT Edge device** configured as a transparent gateway. See [nested-edge-test-environment.md](nested-edge-test-environment.md) for setup instructions.
- **Leaf device** with the ADU agent installed and configured to connect through the gateway.
- **ADU import manifest and update payload** prepared for the target device class.
- **Azure CLI** with the `azure-iot` extension installed:

  ```bash
  az extension add --name azure-iot
  ```

---

## Step 1: Register Devices in IoT Hub

Register both the gateway (IoT Edge) device and the leaf (child) device in your IoT Hub.

### 1.1 Create the IoT Edge Gateway Device

```bash
az iot hub device-identity create \
  --hub-name <hub-name> \
  --device-id <edge-device-id> \
  --edge-enabled
```

### 1.2 Create the Leaf Device as a Child

```bash
az iot hub device-identity create \
  --hub-name <hub-name> \
  --device-id <leaf-device-id>

az iot hub device-identity parent set \
  --hub-name <hub-name> \
  --device-id <leaf-device-id> \
  --parent-device-id <edge-device-id>
```

### 1.3 Set Authentication Method

Choose **SAS** (symmetric key) or **X.509** depending on your scenario:

```bash
# SAS (default)
az iot hub device-identity show \
  --hub-name <hub-name> \
  --device-id <leaf-device-id> \
  --query "authentication"

# X.509 — provide thumbprint at creation time
az iot hub device-identity create \
  --hub-name <hub-name> \
  --device-id <leaf-device-id> \
  --auth-method x509_thumbprint \
  --primary-thumbprint <thumbprint> \
  --secondary-thumbprint <thumbprint>
```

### 1.4 Assign the Leaf Device to a Device Group

Tag the leaf device's twin so ADU can target it in a device group:

```bash
az iot hub device-twin update \
  --hub-name <hub-name> \
  --device-id <leaf-device-id> \
  --tags '{"ADUGroup": "<group-id>"}'
```

---

## Step 2: Configure ADU on the Leaf Device

### 2.1 Edit `du-config.json`

On the leaf device, configure `/etc/adu/du-config.json` to route traffic through the gateway:

```json
{
  "schemaVersion": "1.1",
  "aduShellTrustedUsers": ["adu", "do"],
  "iotHubProtocol": "mqtt",
  "manufacturer": "<device-manufacturer>",
  "model": "<device-model>",
  "agents": [
    {
      "name": "main",
      "runas": "adu",
      "connectionSource": {
        "connectionType": "string",
        "connectionData": "HostName=<hub-name>.azure-devices.net;DeviceId=<leaf-device-id>;SharedAccessKey=<key>;GatewayHostName=<edge-gateway-hostname>"
      },
      "manufacturer": "<device-manufacturer>",
      "model": "<device-model>"
    }
  ],
  "edgegatewayCertPath": "/etc/adu/certs/edge-gateway-ca.pem"
}
```

> **Important:** The `GatewayHostName` in the connection string must match the hostname of the IoT Edge gateway device. The `edgegatewayCertPath` must point to the gateway's root CA certificate installed on the leaf device.

### 2.2 Verify the Agent Starts and Connects

Restart the ADU agent and check its logs:

```bash
sudo systemctl restart adu-agent
sudo journalctl -u adu-agent -f
```

Look for log entries confirming a successful connection to IoT Hub through the gateway.

### 2.3 Verify Device Twin Reports Correctly

Confirm that the device twin reports the correct `manufacturer` and `model`:

```bash
az iot hub device-twin show \
  --hub-name <hub-name> \
  --device-id <leaf-device-id> \
  --query "properties.reported.deviceUpdate"
```

### 2.4 Verify Agent Shows as "Idle" in Portal

In the Azure portal, navigate to **IoT Hub → Device Update → Devices**. The leaf device should appear with a status of **Idle**, indicating it is connected and ready to receive updates.

---

## Step 3: Prepare and Import an Update

### 3.1 Create a Test Update

For testing, create a simple script-based or apt-based update. Below is an example using a shell script handler:

**update-script.sh:**

```bash
#!/bin/bash
echo "Test update v1.1 applied" > /var/log/adu-test-update.log
exit 0
```

### 3.2 Create the Import Manifest

Create an import manifest JSON file that describes the update and its compatibility:

**import-manifest.json:**

```json
{
  "updateId": {
    "provider": "<provider>",
    "name": "<update-name>",
    "version": "1.1"
  },
  "compatibility": [
    {
      "manufacturer": "<device-manufacturer>",
      "model": "<device-model>"
    }
  ],
  "instructions": {
    "steps": [
      {
        "handler": "microsoft/script:1",
        "files": ["update-script.sh"],
        "handlerProperties": {
          "scriptFileName": "update-script.sh",
          "arguments": "--apply"
        }
      }
    ]
  },
  "files": [
    {
      "filename": "update-script.sh",
      "sizeInBytes": <file-size>,
      "hashes": {
        "sha256": "<sha256-hash>"
      }
    }
  ],
  "manifestVersion": "5.0",
  "createdDateTime": "2024-01-01T00:00:00Z"
}
```

### 3.3 Import the Update to ADU

```bash
az iot du update import --account <account> --instance <instance> \
  --import-manifest import-manifest.json --file update-script.sh
```

Verify the import succeeded:

```bash
az iot du update show --account <account> --instance <instance> \
  --update-provider <provider> --update-name <update-name> --update-version 1.1
```

---

## Step 4: Deploy the Update to the Leaf Device

### 4.1 Verify the Device Group

Confirm the leaf device appears in the expected device group:

```bash
az iot du device group show --account <account> --instance <instance> --group-id <group>
```

### 4.2 Create the Deployment

```bash
az iot du deployment create --account <account> --instance <instance> \
  --group-id <group> --deployment-id <id> \
  --update-name <name> --update-version <ver> --update-provider <provider>
```

### 4.3 Monitor Deployment Status

```bash
az iot du deployment show --account <account> --instance <instance> \
  --group-id <group> --deployment-id <id>
```

> **Note:** Deployment status transitions through `Active` → `InProgress` → `Succeeded` (or `Failed`). Allow a few minutes for the leaf device to pick up the deployment.

---

## Step 5: Verify the Update Flow

### 5.1 Check Leaf Device ADU Agent Logs

On the leaf device, inspect the agent logs for the following sequence of events:

```bash
sudo journalctl -u adu-agent --since "10 minutes ago" --no-pager
```

Confirm that each stage completes in order:

| Stage | What to Look For |
|-------|-----------------|
| **Deployment received** | Log entry indicating a new deployment action was received. |
| **Content download initiated** | Download begins; traffic should route through the MCC/gateway. |
| **Download completed** | Payload downloaded successfully with hash verification. |
| **Install step executed** | The update handler runs the install action. |
| **Apply step executed** | The update handler runs the apply action. |
| **Result reported** | Success result is reported back to the ADU service. |

### 5.2 Check Deployment Status via CLI/Portal

```bash
az iot du deployment show --account <account> --instance <instance> \
  --group-id <group> --deployment-id <id> \
  --query "deploymentStatus"
```

The status should report **Succeeded**.

### 5.3 Verify the New Installed Version

Confirm the device twin now reflects the updated version:

```bash
az iot hub device-twin show \
  --hub-name <hub-name> \
  --device-id <leaf-device-id> \
  --query "properties.reported.deviceUpdate.installedUpdateId"
```

---

## Step 6: Verify Content Flows Through the Gateway

This step confirms the leaf device downloaded update content through the gateway rather than directly from the internet.

### 6.1 Check MCC Container Logs on the Gateway

On the gateway device, inspect the Microsoft Connected Cache (MCC) container logs for download requests originating from the leaf device's IP address:

```bash
sudo docker logs mcc-container 2>&1 | grep <leaf-device-ip>
```

You should see HTTP GET requests for the update payload.

### 6.2 Verify No Direct CDN Connections from the Leaf

On the leaf device, check that no outbound connections were made directly to Azure CDN endpoints:

```bash
# Monitor active connections during a deployment
sudo tcpdump -i any host *.delivery.mp.microsoft.com -c 10

# Alternatively, check established connections
sudo netstat -tn | grep -E "443|80"
```

> **Tip:** Run `tcpdump` before starting the deployment so you can capture the full download flow.

### 6.3 Network Isolation Test

For a definitive test, block the leaf device's direct internet access and verify that updates still succeed through the gateway:

```bash
# On the leaf device — block all outbound traffic except to the gateway
sudo iptables -A OUTPUT -d <gateway-ip> -j ACCEPT
sudo iptables -A OUTPUT -d 127.0.0.0/8 -j ACCEPT
sudo iptables -A OUTPUT -j DROP
```

Deploy a new update version and confirm it completes successfully. If it does, content is correctly flowing through the gateway.

> **Important:** Remember to remove the iptables rules after testing:
>
> ```bash
> sudo iptables -F OUTPUT
> ```

---

## Automated Testing (CI/CD Considerations)

### Existing E2E Test Infrastructure

The repository includes end-to-end test infrastructure under `azurepipelines/e2e_test/`. Use it as the foundation for automated nested-edge testing.

### Extending for Nested Edge Scenarios

To add nested-edge coverage to the existing E2E pipeline:

1. **Provision infrastructure** — Add pipeline steps to deploy an IoT Edge gateway VM and a leaf device VM (or container).
2. **Configure the parent-child relationship** — Use Azure CLI commands (as in Steps 1–2) to register devices and set up the hierarchy.
3. **Run the update flow** — Import a test update, create a deployment, and poll for completion.
4. **Assert results** — Verify the deployment status is `Succeeded` and the device twin reports the new version.

### Environment Variables and Test Runner Configuration

| Variable | Description |
|----------|-------------|
| `ADU_ACCOUNT_NAME` | ADU account name |
| `ADU_INSTANCE_NAME` | ADU instance name |
| `IOT_HUB_NAME` | IoT Hub name |
| `EDGE_DEVICE_ID` | Gateway device identity |
| `LEAF_DEVICE_ID` | Leaf device identity |
| `ADU_GROUP_ID` | Target device group ID |
| `UPDATE_PROVIDER` | Update provider string |
| `UPDATE_NAME` | Update name |
| `UPDATE_VERSION` | Update version to deploy |

These can be set in the pipeline's variable group or passed as parameters to the test runner.

---

## Troubleshooting

### Update Stuck in "InProgress"

- Check the ADU agent logs on the leaf device for errors:
  ```bash
  sudo journalctl -u adu-agent --since "30 minutes ago" --no-pager | grep -i error
  ```
- Verify network connectivity from the leaf device to the gateway.
- Ensure the ADU agent process is running: `systemctl status adu-agent`.

### Content Download Fails

- Verify the MCC container is running on the gateway: `sudo docker ps | grep mcc`.
- Check content routing rules — the leaf device must resolve the gateway hostname correctly.
- Inspect MCC logs for HTTP errors: `sudo docker logs mcc-container 2>&1 | tail -50`.

### Device Not Appearing in Device Group

- Confirm the `ADUGroup` tag is set correctly on the device twin:
  ```bash
  az iot hub device-twin show --hub-name <hub-name> --device-id <leaf-device-id> \
    --query "tags.ADUGroup"
  ```
- Wait a few minutes — device group membership may take time to refresh.
- Check that the device's `manufacturer` and `model` in the twin match the update's compatibility info.

### Deployment Reports "Failed"

- Check the result code in the device twin's reported properties:
  ```bash
  az iot hub device-twin show --hub-name <hub-name> --device-id <leaf-device-id> \
    --query "properties.reported.deviceUpdate.lastInstallResult"
  ```
- Common failure causes:
  - **Hash mismatch** — the payload file was modified after manifest creation.
  - **Handler error** — the update script returned a non-zero exit code.
  - **Insufficient permissions** — the ADU agent user cannot execute the handler.
- Review the handler-specific logs on the device for detailed error output.
