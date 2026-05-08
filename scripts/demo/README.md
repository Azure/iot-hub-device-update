# ADU Gen2 Agent — Ignite Demo

End-to-end demo of the ADU Gen2 agent using the **simulator comm provider**
and **file side-loader**. Runs fully offline — no cloud dependencies.

## What It Demonstrates

| Feature | Description |
|---------|-------------|
| **Extensible EDK** | Extensions loaded as `.so` files at runtime |
| **Simulator Comm** | Reads update manifests from a local directory |
| **File Side-loader** | Downloads content via `file://` URIs (local copy) |
| **Script Handler v2** | Executes `install.sh` to apply the firmware |
| **DAG Workflow** | Step 1 depends on Step 0 (`requires` field) |
| **V2 Protocol** | Reports `outcome` / `origin` / `lastInstallResult` |
| **Hash Validation** | SHA-256 verified on all downloaded content |
| **TOML Config** | Agent configured via `.toml` file |

## Prerequisites

Build the agent first:

```bash
cd /path/to/adu-gen2
cmake -B out -DCMAKE_BUILD_TYPE=Debug
cmake --build out
```

## Quick Start

```bash
cd adu-gen2

# 1. Prepare the demo environment
bash scripts/demo/setup_demo.sh

# 2. Run the demo
bash scripts/demo/run_demo.sh

# 3. Clean up
bash scripts/demo/cleanup_demo.sh
```

## Files

| File | Purpose |
|------|---------|
| `setup_demo.sh` | Creates directories, content, manifest, and config |
| `run_demo.sh` | Runs the agent and displays results |
| `cleanup_demo.sh` | Removes all demo temp files |
| `content/install.sh` | Sample firmware install script |

## Environment Variables

The simulator comm provider reads these (set automatically by `setup_demo.sh`):

| Variable | Description |
|----------|-------------|
| `ADUC_SIM_MANIFEST_DIR` | Directory containing `.json` manifests |
| `ADUC_SIM_CONTENT_DIR` | Directory containing payload files |
| `ADUC_SIM_RESULT_DIR` | Directory where results are written |

## Manifest Structure

The demo creates a v5 update manifest with:

- **Update ID**: `contoso / toaster-firmware / 2.0.0`
- **Step 0** — `install-firmware`: Runs `install.sh` with the firmware binary
- **Step 1** — `verify-install`: Runs after Step 0 completes (`requires: install-firmware`)
- **Files**: `firmware-v2.0.0.bin` (binary) + `install.sh` (script), both SHA-256 verified
