# Mock IoT Hub Server

A Python HTTP server (stdlib only, no dependencies) that simulates IoT Hub device twin and direct methods for testing the ADU Gen2 agent's IoT Hub communication adapter.

## Quick Start

```bash
# Basic usage with default manifest
python server.py --port 8081 --verbose

# With a custom manifest and payload files
python server.py --port 8081 --manifest-file ../demo/manifests/multi_step_manifest.json --content-dir ../demo/payloads --verbose

# No deployment (device twin has no pending update)
python server.py --no-deployment
```

## Endpoints

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/health` | Health check |
| `GET` | `/twins/{deviceId}` | Get full device twin (desired + reported) |
| `PATCH` | `/twins/{deviceId}/properties/reported` | Update reported properties |
| `POST` | `/twins/{deviceId}/methods/{methodName}` | Invoke direct method (server→device) |
| `GET` | `/devices/{deviceId}/deployments` | Get pending ADU deployment manifest |
| `POST` | `/devices/{deviceId}/deployments/{workflowId}/result` | Report deployment result |
| `GET` | `/devices/{deviceId}/files/{fileId}/url` | Get file download URL |
| `GET` | `/files/{fileId}/content` | Download file content |
| `POST` | `/admin/trigger-deployment` | Push new deployment to device(s) |
| `GET` | `/admin/status` | View all device states and interactions |

## Device Twin Structure

```json
{
    "deviceId": "test-device-001",
    "properties": {
        "desired": {
            "ADUGroup": "test-group",
            "deployment": {
                "workflowId": "wf-multi-001",
                "manifest": { ... }
            },
            "$version": 1
        },
        "reported": {
            "ADUAgent": {
                "state": "Idle",
                "lastDeploymentId": "",
                "installedUpdateId": ""
            },
            "$version": 1
        }
    }
}
```

## CLI Arguments

| Argument | Default | Description |
|----------|---------|-------------|
| `--port` | 8081 | Port to listen on |
| `--manifest-file` | (built-in) | Path to deployment manifest JSON |
| `--content-dir` | (none) | Directory to serve payload files from |
| `--device-id` | test-device-001 | Default device ID to initialize |
| `--no-deployment` | false | Start with no pending deployment |
| `--verbose` | false | Enable debug-level logging |

## Admin API

### Trigger a new deployment

```bash
curl -X POST http://localhost:8081/admin/trigger-deployment \
  -H "Content-Type: application/json" \
  -d '{"manifest": {"workflowId": "wf-new", ...}, "deviceId": "my-device"}'
```

If `deviceId` is omitted, the deployment is pushed to all known devices.

### Check status

```bash
curl http://localhost:8081/admin/status | python -m json.tool
```

## Multi-Device Support

The server automatically creates device twins on first access. Any device ID used in API calls will get its own twin instance with the current deployment manifest.

## Integration with ADU Gen2 Agent

Configure the agent to use `adu_direct` communication pointing at this server:

```toml
[communication]
type = "adu_direct"
endpoint = "http://localhost:8081"
poll_interval_sec = 5
```

The server also supports IoT Hub-style twin endpoints for the `iothub` communication adapter.
