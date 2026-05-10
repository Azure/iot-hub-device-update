# Mock ADU Server

A lightweight mock server that simulates the Azure Device Update (ADU) REST API
for testing the Gen2 agent's Direct communication provider.

**No external dependencies** — uses only Python's standard library (`http.server`).

## Quick Start

```bash
# Start with default settings (port 8080, built-in manifest)
python server.py

# Verify it's running
curl http://localhost:8080/health
```

## Endpoints

| Method | Path | Description |
|--------|------|-------------|
| GET | `/health` | Health check → `{"status": "ok"}` |
| GET | `/devices/{deviceId}/deployments?api-version=2024-01-01` | Get pending deployment manifest (or 204 if none) |
| PUT | `/devices/{deviceId}/state` | Report device state (logged by server) |
| POST | `/devices/{deviceId}/deployments/{workflowId}/result` | Report deployment result (marks complete) |
| GET | `/devices/{deviceId}/files/{fileId}/url` | Get download URL for a file |
| GET | `/files/{fileId}/content` | Download file content |

## Command-Line Options

```
--port PORT        Port to listen on (default: 8080)
--manifest-file    Path to a JSON file with a custom deployment manifest
--content-dir      Directory to serve update payload files from
--no-deployment    Start with no pending deployment (returns 204)
--verbose          Log all HTTP requests
```

## Examples

### Basic usage with verbose logging

```bash
python server.py --verbose
```

### Custom port, no deployment initially

```bash
python server.py --port 9090 --no-deployment
```

### Custom manifest and payload files

```bash
python server.py --manifest-file ./my_manifest.json --content-dir ./payloads/
```

### Testing with curl

```bash
# Check health
curl http://localhost:8080/health

# Poll for deployment
curl http://localhost:8080/devices/my-device-01/deployments?api-version=2024-01-01

# Report device state
curl -X PUT http://localhost:8080/devices/my-device-01/state \
  -H "Content-Type: application/json" \
  -d '{"state": "idle", "installedVersion": "1.0.0"}'

# Get file download URL
curl http://localhost:8080/devices/my-device-01/files/f1/url

# Download file content
curl http://localhost:8080/files/f1/content -o install.sh

# Report deployment result (marks deployment complete)
curl -X POST http://localhost:8080/devices/my-device-01/deployments/wf-001/result \
  -H "Content-Type: application/json" \
  -d '{"resultCode": 0, "extendedResultCode": 0}'

# Subsequent polls return 204 (no content) after completion
curl -v http://localhost:8080/devices/my-device-01/deployments?api-version=2024-01-01
```

## Deployment Lifecycle

1. Agent polls `GET /devices/{id}/deployments` → receives manifest (200)
2. Agent downloads files via `GET /devices/{id}/files/{fileId}/url` → gets URL
3. Agent downloads content from the returned URL
4. Agent reports state via `PUT /devices/{id}/state`
5. Agent reports result via `POST /devices/{id}/deployments/{wfId}/result`
6. Subsequent polls return 204 (deployment complete)

## Custom Manifest Format

```json
{
  "workflowId": "wf-001",
  "updateId": {"provider": "Contoso", "name": "VacuumFirmware", "version": "2.0.0"},
  "instructions": {
    "steps": [
      {
        "type": "inline",
        "handler": "microsoft/script:1",
        "handlerProperties": {"scriptFileName": "install.sh", "arguments": "--force"},
        "files": ["f1"],
        "installedCriteria": "2.0.0"
      }
    ]
  },
  "files": {
    "f1": {
      "fileName": "install.sh",
      "sizeInBytes": 1024,
      "hashes": {"sha256": "abc123fake"}
    }
  }
}
```
