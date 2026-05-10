# ADU Gen2 Agent — Ignite Demo Script

## What We're Showing

The ADU Gen2 Agent: a ground-up rewrite of the Azure Device Update agent in pure C99
with a modular extension architecture. This demo shows the full update lifecycle from
cloud signal to device execution.

## Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                        ADU Cloud Service                            │
│                    (Mock Server for Demo)                           │
└──────────────────────────────┬──────────────────────────────────────┘
                               │  REST API (mTLS)
                               ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      ADU Gen2 Agent                                  │
│                                                                     │
│  ┌──────────────┐  ┌──────────────────┐  ┌──────────────────────┐  │
│  │  Comm Layer  │  │  Workflow Engine  │  │  Extension Manager   │  │
│  │  (adu_direct │  │  (DAG-based,     │  │  (dlopen, vtables,   │  │
│  │   or iothub) │  │   8-phase steps) │  │   hot-reload)        │  │
│  └──────┬───────┘  └────────┬─────────┘  └──────────┬───────────┘  │
│         │                   │                        │              │
│         ▼                   ▼                        ▼              │
│  ┌──────────────┐  ┌──────────────────┐  ┌──────────────────────┐  │
│  │  ADU Direct  │  │  Step Handlers   │  │  Component Enum      │  │
│  │  IoT Hub     │  │  • Simulator     │  │  • Static File       │  │
│  │  (adapters)  │  │  • SWUpdate      │  │  • Contoso           │  │
│  └──────────────┘  └──────────────────┘  └──────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
```

## Prerequisites

```bash
# Build the agent (WSL2 Ubuntu 24.04)
cd adu-gen2
cmake -B out/build -G Ninja -DCMAKE_C_COMPILER=gcc-12
cmake --build out/build

# Verify extensions built
ls out/build/src/extensions/**/*.so
```

## Step-by-Step Demo Commands

### 1. Start the Mock ADU Server

```bash
cd tools/mock_adu_server
python3 server.py --port 8199
```

**Talking Point:** The mock server simulates the ADU cloud service REST API.
It serves deployment manifests and accepts status reports — identical protocol
to production.

### 2. Start the Agent

```bash
cd tools/demo
./run_demo.sh --build-dir ../../out/build
```

**Talking Point:** The agent starts, loads its config, discovers extensions via
dlopen, and begins polling. Watch the log output — you'll see each extension
load with its capabilities.

### 3. Observe Extension Loading

Expected output:
```
[INFO] Extension loaded: adu-direct-comm (communication)
[INFO] Extension loaded: iothub-adapter (communication, stub)
[INFO] Extension loaded: gen2-simulator (step_handler: microsoft/simulator:1)
[INFO] Extension loaded: swupdate-adapter (step_handler: microsoft/swupdate:2)
[INFO] Component enumerator: 2 components discovered
[INFO] Agent ready. Polling http://127.0.0.1:8199 every 5s
```

**Talking Point:** Extensions are shared libraries with a single export function.
The agent loads them dynamically — no recompilation needed to add new handlers.
The backward-compat adapters (IoT Hub, SWUpdate) show how we bridge Gen1 ↔ Gen2.

### 4. Trigger a Deployment

```bash
# In another terminal — push a deployment to mock server
curl -X POST http://127.0.0.1:8199/api/deployments \
  -H "Content-Type: application/json" \
  -d @sample-manifest.json
```

**Talking Point:** The deployment manifest specifies steps with handler types.
The workflow engine matches `microsoft/simulator:1` to the simulator handler
and executes the 8-phase lifecycle.

### 5. Watch the Workflow Execute

Expected output:
```
[INFO] Deployment received: workflow-001
[INFO] Step 1/1: Evaluate → Acquire → Preprocess → Execute → Validate → Postprocess
[INFO] gen2-simulator: Executing (0% → 25% → 50% → 75% → 100%)
[INFO] Step 1/1: SUCCESS
[INFO] Deployment workflow-001: SUCCEEDED
[INFO] Reporting result to service...
```

**Talking Point:** Each step goes through all 8 phases. The DAG engine handles
dependencies between steps. Progress is reported in real-time. On failure,
automatic rollback triggers Postprocess with `rollback=true`.

### 6. Verify Status

```bash
./adu_status_check.sh
```

**Talking Point:** The agent exposes a local Unix socket API for status queries.
This is the same interface fleet management tools use for device health monitoring.

### 7. Clean Shutdown

Press `Ctrl+C` in the demo terminal.

**Talking Point:** The agent performs graceful shutdown: finishes in-progress
phases, persists workflow state (so it can resume after reboot), unloads
extensions in reverse order.

## Key Talking Points

1. **Pure C99** — No C++ runtime, ~200KB binary, boots in <100ms on embedded devices
2. **Extension Model** — dlopen-based, versioned vtables, backward-compatible
3. **8-Phase Lifecycle** — Evaluate/Acquire/Preprocess/Execute/Validate/Postprocess/Report/Signal
4. **DAG Workflow** — Steps can have dependencies, parallel execution where possible
5. **Backward Compat** — Gen1 IoT Hub and SWUpdate handlers wrapped as Gen2 extensions
6. **mTLS** — Certificate-based auth to ADU service (no connection strings)
7. **Binary Logging** — Format strings never stored in logs (security + performance)
8. **Crash Recovery** — Workflow state persisted to SQLite, auto-resume after reboot

## Expected Output Summary

| Phase | What Happens | Duration |
|-------|-------------|----------|
| Boot | Load config, discover extensions | <100ms |
| Poll | HTTP GET to service endpoint | ~50ms |
| Deploy | Receive manifest, parse steps | ~10ms |
| Execute | Run step handler lifecycle | ~500ms (sim) |
| Report | HTTP POST result to service | ~50ms |
