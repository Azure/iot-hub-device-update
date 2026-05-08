#!/usr/bin/env bash
# setup_demo.sh — Prepare the ADU Gen2 Ignite demo environment.
#
# Creates all directories, content files, manifests, and config needed
# to run the agent with the simulator comm provider fully offline.
#
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

set -euo pipefail

# ─── Paths ──────────────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
DEMO_BASE="${SCRIPT_DIR}/.demo-env"

MANIFEST_DIR="${DEMO_BASE}/manifests"
CONTENT_DIR="${DEMO_BASE}/content"
RESULT_DIR="${DEMO_BASE}/results"
WORK_DIR="${DEMO_BASE}/work"
INSTALL_TARGET="${DEMO_BASE}/installed"
CONFIG_FILE="${DEMO_BASE}/adu-agent.toml"

AGENT_BINARY="${REPO_ROOT}/out/bin/adu_gen2_agent"

# Extensions — pick from the build output tree
EXT_BASE="${REPO_ROOT}/out/src/extensions"
EXT_DIRS=(
    "${EXT_BASE}/communication_providers/simulator_comm"
    "${EXT_BASE}/content_downloaders/file_sideloader"
    "${EXT_BASE}/content_processors/delta_processor"
    "${EXT_BASE}/step_handlers/script_handler_v2"
    "${EXT_BASE}/step_handlers/apt_handler_v2"
)

# ─── Colors ─────────────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BLUE='\033[0;34m'; MAGENTA='\033[0;35m'
BOLD='\033[1m'; DIM='\033[2m'; RESET='\033[0m'

banner()  { echo -e "\n${CYAN}${BOLD}$1${RESET}"; }
info()    { echo -e "  ${GREEN}✓${RESET} $1"; }
detail()  { echo -e "  ${DIM}  $1${RESET}"; }
warn()    { echo -e "  ${YELLOW}⚠${RESET} $1"; }

# ═══════════════════════════════════════════════════════════════════════════
echo -e "${MAGENTA}${BOLD}"
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║                                                         ║"
echo "║     ADU Gen2 Agent — Ignite Demo Setup                  ║"
echo "║     Fully-offline simulator environment                 ║"
echo "║                                                         ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo -e "${RESET}"

# ─── 1. Create directory structure ──────────────────────────────────────────
banner "📁  Creating demo directories..."

rm -rf "${DEMO_BASE}"
mkdir -p "${MANIFEST_DIR}" "${CONTENT_DIR}" "${RESULT_DIR}" "${WORK_DIR}" "${INSTALL_TARGET}"

info "Manifests:  ${MANIFEST_DIR}"
info "Content:    ${CONTENT_DIR}"
info "Results:    ${RESULT_DIR}"
info "Work:       ${WORK_DIR}"
info "Install:    ${INSTALL_TARGET}"

# ─── 2. Create content files ───────────────────────────────────────────────
banner "📦  Creating test content files..."

# Fake firmware binary (1 KB of deterministic data)
dd if=/dev/urandom bs=1024 count=1 of="${CONTENT_DIR}/firmware-v2.0.0.bin" 2>/dev/null
info "firmware-v2.0.0.bin  (1024 bytes — simulated firmware image)"

# Install script
cp "${SCRIPT_DIR}/content/install.sh" "${CONTENT_DIR}/install.sh"
chmod +x "${CONTENT_DIR}/install.sh"
info "install.sh           (firmware installer script)"

# ─── 3. Compute SHA-256 hashes ─────────────────────────────────────────────
banner "🔒  Computing SHA-256 hashes..."

HASH_FIRMWARE=$(sha256sum "${CONTENT_DIR}/firmware-v2.0.0.bin" | awk '{print $1}')
HASH_INSTALL=$(sha256sum  "${CONTENT_DIR}/install.sh"          | awk '{print $1}')
SIZE_FIRMWARE=$(stat -c%s "${CONTENT_DIR}/firmware-v2.0.0.bin")
SIZE_INSTALL=$(stat -c%s  "${CONTENT_DIR}/install.sh")

info "firmware-v2.0.0.bin  ${HASH_FIRMWARE:0:16}..."
info "install.sh           ${HASH_INSTALL:0:16}..."

# ─── 4. Create v5 update manifest ──────────────────────────────────────────
banner "📝  Creating v5 update manifest..."

cat > "${MANIFEST_DIR}/ignite-demo-manifest.json" <<MANIFEST
{
    "workflowId": "wf-ignite-demo-2025",
    "updateId": {
        "provider": "contoso",
        "name": "toaster-firmware",
        "version": "2.0.0"
    },
    "instructions": {
        "steps": [
            {
                "type": "inline",
                "handler": "microsoft/script:2",
                "handlerProperties": {
                    "id": "install-firmware",
                    "scriptFileName": "install.sh",
                    "arguments": "firmware-v2.0.0.bin",
                    "timeout": 120
                },
                "files": ["install.sh", "firmware-v2.0.0.bin"],
                "installedCriteria": "contoso-toaster-fw-2.0.0"
            },
            {
                "type": "inline",
                "handler": "microsoft/script:2",
                "handlerProperties": {
                    "id": "verify-install",
                    "requires": "install-firmware",
                    "scriptFileName": "install.sh",
                    "arguments": "--verify-only",
                    "timeout": 30
                },
                "files": ["install.sh"],
                "installedCriteria": "contoso-toaster-verify-2.0.0"
            }
        ]
    },
    "files": {
        "firmware-v2.0.0.bin": {
            "fileName": "firmware-v2.0.0.bin",
            "sizeInBytes": ${SIZE_FIRMWARE},
            "hashes": {
                "sha256": "${HASH_FIRMWARE}"
            }
        },
        "install.sh": {
            "fileName": "install.sh",
            "sizeInBytes": ${SIZE_INSTALL},
            "hashes": {
                "sha256": "${HASH_INSTALL}"
            }
        }
    },
    "compatibility": [
        {
            "manufacturer": "contoso",
            "model": "toaster-900x"
        }
    ]
}
MANIFEST

info "Manifest written: ignite-demo-manifest.json"
detail "Provider: contoso | Name: toaster-firmware | Version: 2.0.0"
detail "Step 0: install-firmware  (microsoft/script:2)"
detail "Step 1: verify-install    (microsoft/script:2, requires: install-firmware)"
detail "Files: firmware-v2.0.0.bin, install.sh"

# ─── 5. Create agent TOML config ──────────────────────────────────────────
banner "⚙️   Creating agent configuration..."

# Create a flat directory with symlinks to all extension .so files
EXT_FLAT="${DEMO_BASE}/extensions"
mkdir -p "${EXT_FLAT}"
for ext_dir in "${EXT_DIRS[@]}"; do
    if [ -d "${ext_dir}" ]; then
        for so_file in "${ext_dir}"/lib*.so; do
            if [ -f "${so_file}" ]; then
                ln -sf "${so_file}" "${EXT_FLAT}/$(basename "${so_file}")"
            fi
        done
    fi
done

EXT_SEARCH_PATH="${EXT_FLAT}"

cat > "${CONFIG_FILE}" <<TOML
# ADU Gen2 Agent Configuration — Ignite Demo
# Auto-generated by setup_demo.sh

[agent]
name = "adu-gen2-ignite-demo"
device_id = "ignite-demo-device-001"
manufacturer = "contoso"
model = "toaster-900x"
mode = "once"

[communication]
type = "simulator"
endpoint = "file://localhost"
poll_interval_sec = 2

[extensions]
search_path = "${EXT_SEARCH_PATH}"

[logging]
level = 6
file = "${DEMO_BASE}/adu-agent.log"
console = true

[download]
work_dir = "${WORK_DIR}"
max_retries = 3
TOML

info "Config written: ${CONFIG_FILE}"
detail "Device: ignite-demo-device-001"
detail "Mode: once (single poll cycle)"
detail "Extensions: search_path → ${EXT_SEARCH_PATH}"

# ─── 6. Verify prerequisites ──────────────────────────────────────────────
banner "🔍  Verifying prerequisites..."

if [ -x "${AGENT_BINARY}" ]; then
    info "Agent binary: ${AGENT_BINARY}"
else
    warn "Agent binary not found: ${AGENT_BINARY}"
    warn "Build with: cd ${REPO_ROOT} && cmake --build out"
fi

EXT_COUNT=0
for ext_dir in "${EXT_DIRS[@]}"; do
    if [ -d "${ext_dir}" ]; then
        for so_file in "${ext_dir}"/lib*.so; do
            if [ -f "${so_file}" ]; then
                info "Extension: $(basename "${so_file}")"
                EXT_COUNT=$((EXT_COUNT + 1))
            fi
        done
    fi
done
if [ $EXT_COUNT -eq 0 ]; then
    warn "No extension .so files found — build the project first"
fi

# ─── 7. Print environment summary ─────────────────────────────────────────
banner "📋  Environment variables for the demo:"

echo ""
echo -e "  ${BOLD}export ADUC_SIM_MANIFEST_DIR=${MANIFEST_DIR}${RESET}"
echo -e "  ${BOLD}export ADUC_SIM_CONTENT_DIR=${CONTENT_DIR}${RESET}"
echo -e "  ${BOLD}export ADUC_SIM_RESULT_DIR=${RESULT_DIR}${RESET}"
echo -e "  ${BOLD}export ADUC_INSTALL_TARGET=${INSTALL_TARGET}${RESET}"
echo ""

# Write an env file for convenience
cat > "${DEMO_BASE}/demo.env" <<ENV
export ADUC_SIM_MANIFEST_DIR=${MANIFEST_DIR}
export ADUC_SIM_CONTENT_DIR=${CONTENT_DIR}
export ADUC_SIM_RESULT_DIR=${RESULT_DIR}
export ADUC_INSTALL_TARGET=${INSTALL_TARGET}
ENV
info "Env file: ${DEMO_BASE}/demo.env  (source this before running)"

# ─── Done ──────────────────────────────────────────────────────────────────
echo ""
echo -e "${GREEN}${BOLD}╔═══════════════════════════════════════════════════════════╗${RESET}"
echo -e "${GREEN}${BOLD}║  ✅  Demo environment ready!                             ║${RESET}"
echo -e "${GREEN}${BOLD}║                                                          ║${RESET}"
echo -e "${GREEN}${BOLD}║  Next:  bash scripts/demo/run_demo.sh                    ║${RESET}"
echo -e "${GREEN}${BOLD}╚═══════════════════════════════════════════════════════════╝${RESET}"
echo ""
