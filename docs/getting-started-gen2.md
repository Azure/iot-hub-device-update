# Getting Started with ADU Gen2 Agent

## Prerequisites

- Ubuntu 22.04+ (primary dev platform)
- CMake 3.16+, Ninja
- GCC or Clang with C11 support
- libcurl, OpenSSL, pthread
- Python 3.8+ (for mock server and tools)

## Building the Agent

### Install Dependencies

```bash
./scripts/install-deps.sh -a
```

### Build

```bash
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DADUC_ENABLE_COVERAGE=ON

ninja -C build
```

Output binaries are in `out/bin/`. Extensions (.so) are in `out/lib/extensions/`.

### Run Tests

```bash
ctest --test-dir build --output-on-failure
```

159 unit tests, 763 assertions across the SDK, DAG engine, and handlers.

### Build .deb Package

```bash
./scripts/build.sh -c -u --build-packages
sudo apt install ./out/deviceupdate-agent_*.deb
```

---

## Running with the Mock Server

The fastest way to see the agent in action:

```bash
# Full automated demo (builds, starts mock server, runs agent)
./tools/demo/run_full_demo.sh
```

### Manual Steps

1. **Start mock ADU server:**

```bash
python3 tools/mock_adu_server/server.py \
  --port 8080 \
  --manifest tools/demo/manifests/multi_step_manifest.json \
  --payloads tools/demo/payloads/
```

2. **Verify server health:**

```bash
curl http://localhost:8080/health
```

3. **Run the agent:**

```bash
out/bin/adu_gen2_agent \
  --config tools/demo/adu-agent-demo.conf \
  --once \
  --log-level 6
```

4. **Check deployment status:**

```bash
curl http://localhost:8080/admin/status
```

---

## Configuration Reference (TOML)

The agent reads a TOML config file (`--config` flag).

### Minimal Config

```toml
[agent]
device_id = "my-device-001"
mode = "once"              # "once" = single poll, "daemon" = continuous

[communication]
type = "adu_direct"
endpoint = "http://localhost:8080"
poll_interval_sec = 10

[extensions]
search_path = "./extensions"

[logging]
level = 5                  # 0=FATAL .. 6=TRACE
console = true
```

### Full Config

```toml
[agent]
name = "adu-gen2-agent"
device_id = "my-device-001"
manufacturer = "Contoso"
model = "SmartVacuum"
mode = "daemon"

[communication]
type = "adu_direct"        # or "iothub_adapter"
endpoint = "https://adu.azure-devices.net"
provider = "adu-direct"
poll_interval_sec = 30

[extensions]
search_path = "/usr/lib/adu/extensions"
# Or explicitly list:
# load = ["/usr/lib/adu/extensions/script_handler_v2.so"]

[download]
work_dir = "/var/lib/adu/downloads"
max_retries = 3

[logging]
level = 5
file = "/var/log/adu/adu-gen2.log"
console = false

[security]
cert_path = "/etc/adu/certs/device.pem"
key_path = "/etc/adu/certs/device.key"
```

### Config Fields

| Section | Key | Description |
|---------|-----|-------------|
| `agent` | `device_id` | Device identity for ADU service |
| `agent` | `mode` | `once` (single poll) or `daemon` (continuous) |
| `agent` | `manufacturer` | Device manufacturer (for targeting) |
| `agent` | `model` | Device model (for targeting) |
| `communication` | `type` | Communication provider name |
| `communication` | `endpoint` | ADU service URL |
| `communication` | `poll_interval_sec` | Seconds between polls |
| `extensions` | `search_path` | Directory to scan for .so extensions |
| `download` | `work_dir` | Temp directory for downloads |
| `download` | `max_retries` | Download retry count |
| `logging` | `level` | 0=FATAL, 1=ERROR, 2=WARN, 3=INFO, 4=NOTICE, 5=DEBUG, 6=TRACE |
| `logging` | `file` | Log file path |
| `logging` | `console` | Enable stdout logging |

---

## Writing a Custom Extension

### 1. Create the Handler

```c
// my_handler.c
#include <aduc/extension_sdk.h>

static ADUC_Result MyHandler_Evaluate(ADUC_WorkflowHandle wf, ADUC_StepHandle step) {
    // Check if update is already applied
    return ADUC_Result_Success;  // Proceed with update
}

static ADUC_Result MyHandler_Execute(ADUC_WorkflowHandle wf, ADUC_StepHandle step) {
    const char* content_dir = ADUC_Step_GetContentDir(step);
    // Apply your update logic using files in content_dir
    return ADUC_Result_Success;
}

// Implement other vtable functions...

static ADUC_StepHandler_VTable my_vtable = {
    .Evaluate    = MyHandler_Evaluate,
    .Acquire     = MyHandler_Acquire,
    .Preprocess  = MyHandler_Preprocess,
    .Execute     = MyHandler_Execute,
    .Validate    = MyHandler_Validate,
    .Postprocess = MyHandler_Postprocess,
    .Cancel      = MyHandler_Cancel,
    .Cleanup     = MyHandler_Cleanup,
};

ADUC_EXTENSION_EXPORT ADUC_Result ADUC_Extension_Register(ADUC_ExtensionRegistration* reg) {
    reg->name = "myorg/myhandler";
    reg->version = 1;
    reg->type = ADUC_EXTENSION_TYPE_STEP_HANDLER;
    reg->vtable = &my_vtable;
    return ADUC_Result_Success;
}
```

### 2. Build as Shared Library

```cmake
add_library(my_handler SHARED my_handler.c)
target_link_libraries(my_handler PRIVATE aduc_extension_sdk)
```

### 3. Deploy

Place the `.so` in the extensions search path configured in `adu-agent.conf`.

### 4. Reference in Manifest

```json
{
  "handler": "myorg/myhandler:1",
  "handlerProperties": { "id": "my-step" },
  "files": ["payload.bin"]
}
```

---

## Mock Server Features

The mock server (`tools/mock_adu_server/server.py`) provides:

- Full ADU REST API simulation
- Deployment manifest delivery
- File download serving
- Status reporting endpoint
- Admin API (`/admin/status`) for inspecting agent interactions

The mock IoT Hub (`tools/mock_iothub/server.py`) provides:

- Device twin read/write simulation
- Direct method invocation
- Message routing

---

## Next Steps

- [Architecture Deep Dive](./gen2-architecture.md)
- [Diagnostics & Troubleshooting](./diagnostics.md)
- [Demo Walkthrough](../tools/demo/README.md)
