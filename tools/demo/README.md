# ADU Gen2 Agent — Ignite Demo

## Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                      Demo Architecture                         │
├──────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌─────────────┐     poll      ┌──────────────────────┐      │
│  │  ADU Gen2   │──────────────▶│  Mock ADU Server     │      │
│  │   Agent     │◀──────────────│  (or Mock IoT Hub)   │      │
│  └──────┬──────┘   manifest    └──────────┬───────────┘      │
│         │                                  │                  │
│         │ load extension                   │ serve files      │
│         ▼                                  ▼                  │
│  ┌─────────────┐              ┌──────────────────────┐       │
│  │  Handlers   │              │  payloads/           │       │
│  │  (plugins)  │              │  ├── pre_check.sh    │       │
│  │             │              │  ├── install_fw.sh   │       │
│  │ script:2    │              │  ├── apply_config.sh │       │
│  │ apt:2       │              │  ├── health_check.sh │       │
│  │ swupdate:2  │              │  └── ...             │       │
│  └──────┬──────┘              └──────────────────────┘       │
│         │                                                     │
│         │ DAG execution                                       │
│         ▼                                                     │
│  ┌─────────────────────────────────────────────┐             │
│  │  Step Execution (DAG-ordered):              │             │
│  │                                             │             │
│  │  pre-check ──┬──▶ firmware ──┐              │             │
│  │              │                ├──▶ config ──▶ verify      │
│  │              └──▶ packages ──┘              │             │
│  │                                             │             │
│  │  rollback (runs only on failure)            │             │
│  └─────────────────────────────────────────────┘             │
└──────────────────────────────────────────────────────────────┘
```

## Quick Start

### Option A: Full automated demo

```bash
chmod +x run_full_demo.sh
./run_full_demo.sh
```

### Option B: Manual step-by-step

1. Start the mock server (choose one):
   ```bash
   # ADU Direct mock (original)
   cd tools/mock_adu_server
   python3 server.py --manifest-file ../demo/manifests/multi_step_manifest.json \
       --content-dir ../demo/payloads --verbose

   # IoT Hub mock (device twin + methods)
   cd tools/mock_iothub
   python3 server.py --port 8081 --manifest-file ../demo/manifests/multi_step_manifest.json \
       --content-dir ../demo/payloads --verbose
   ```

2. Run the agent:
   ```bash
   cd out
   ./bin/adu_gen2_agent --config ../tools/demo/adu-agent-demo.conf --once
   ```

3. Watch the agent:
   - Poll the mock server for pending deployments
   - Receive and parse the multi-step manifest
   - Build DAG from step dependencies
   - Execute steps in topological order (parallel where possible)
   - Report result back to server

## Manifests

### `manifests/multi_step_manifest.json` — Full DAG Demo

Showcases ALL handler types with DAG dependencies:

| Step ID | Handler | Depends On | Description |
|---------|---------|------------|-------------|
| `pre-check` | script:2 | — | Pre-flight validation |
| `firmware` | script:2 | pre-check | Firmware flash with rollback |
| `packages` | apt:2 | pre-check | APT package install |
| `config` | script:2 | firmware, packages | Apply configuration |
| `verify` | script:2 | firmware, packages, config | Post-update health check |
| `rollback` | script:2 | *(runOnFailed)* | Full rollback on failure |

DAG execution order:
```
pre-check → [firmware, packages] (parallel) → config → verify
                                                ↓ (on failure)
                                             rollback
```

### `manifests/simple_script_manifest.json` — Single Step

Minimal manifest with one script step. Good for testing basic flow.

### `manifests/swupdate_manifest.json` — SWUpdate Handler

Demonstrates the SWUpdate handler for full firmware image updates with reboot.

## Configuration

The `adu-agent-demo.conf` configures the agent for demo mode:
- `mode = "once"` — single poll cycle, then exit
- `endpoint = "http://localhost:8080"` — points to mock server
- `poll_interval_sec = 5` — fast polling for demo

## What This Demonstrates

- **DAG-based orchestration**: Steps declare dependencies; engine resolves execution order
- **Parallel execution**: Independent steps (firmware + packages) run concurrently
- **Multiple handler types**: script:2, apt:2, swupdate:2 — all as loadable extensions
- **Rollback paths**: `rollbackScript`, `runOnFailed`, `skipOnFailed` for resilient updates
- **Rich results**: Steps return structured results (result code, details, signals)
- **Extensible architecture**: Handlers loaded as .so plugins via TOML config
- **Communication abstraction**: ADU Direct or IoT Hub — swappable providers
- **TOML configuration**: Human-readable, layered config
- **Binary logging**: Compact ETW-inspired format with structured fields

## Notes

- On Linux, run `chmod +x payloads/*.sh run_full_demo.sh` to make scripts executable
- The mock servers use Python stdlib only (no pip install needed)
- All demo scripts are safe — they simulate operations with echo/sleep, no real changes
