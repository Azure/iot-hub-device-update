#!/usr/bin/env bash
# run_demo.sh — Run the ADU Gen2 Ignite demo.
#
# Uses the simulator comm provider with file:// sideloader for a
# fully-offline end-to-end update deployment.
#
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

set -euo pipefail

# ─── Paths ──────────────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
DEMO_BASE="${SCRIPT_DIR}/.demo-env"

CONFIG_FILE="${DEMO_BASE}/adu-agent.toml"
AGENT_BINARY="${REPO_ROOT}/out/bin/adu_gen2_agent"
ENV_FILE="${DEMO_BASE}/demo.env"

# ─── Colors ─────────────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BLUE='\033[0;34m'; MAGENTA='\033[0;35m'
BOLD='\033[1m'; DIM='\033[2m'; RESET='\033[0m'

section() { echo -e "\n${CYAN}${BOLD}━━━ $1 ━━━${RESET}"; }

# ═══════════════════════════════════════════════════════════════════════════
echo -e "${MAGENTA}${BOLD}"
cat << 'BANNER'

     █████╗ ██████╗ ██╗   ██╗     ██████╗ ███████╗███╗   ██╗██████╗
    ██╔══██╗██╔══██╗██║   ██║    ██╔════╝ ██╔════╝████╗  ██║╚════██╗
    ███████║██║  ██║██║   ██║    ██║  ███╗█████╗  ██╔██╗ ██║ █████╔╝
    ██╔══██║██║  ██║██║   ██║    ██║   ██║██╔══╝  ██║╚██╗██║██╔═══╝
    ██║  ██║██████╔╝╚██████╔╝    ╚██████╔╝███████╗██║ ╚████║███████╗
    ╚═╝  ╚═╝╚═════╝  ╚═════╝      ╚═════╝ ╚══════╝╚═╝  ╚═══╝╚══════╝

BANNER
echo -e "${RESET}"
echo -e "${BOLD}    Azure Device Update — Gen2 Agent  ·  Ignite Demo${RESET}"
echo -e "${DIM}    Extensible EDK  ·  DAG Workflows  ·  Offline Simulator${RESET}"
echo ""

# ─── Preflight checks ──────────────────────────────────────────────────────
if [ ! -d "${DEMO_BASE}" ]; then
    echo -e "${RED}ERROR: Demo environment not found.${RESET}"
    echo "  Run:  bash scripts/demo/setup_demo.sh"
    exit 1
fi

if [ ! -f "${ENV_FILE}" ]; then
    echo -e "${RED}ERROR: demo.env not found. Re-run setup_demo.sh${RESET}"
    exit 1
fi

# shellcheck disable=SC1090
source "${ENV_FILE}"

if [ ! -x "${AGENT_BINARY}" ]; then
    echo -e "${RED}ERROR: Agent binary not found: ${AGENT_BINARY}${RESET}"
    echo "  Build with: cd ${REPO_ROOT} && cmake --build out"
    exit 1
fi

# ─── Show configuration ────────────────────────────────────────────────────
section "Agent Configuration"
echo ""
echo -e "${DIM}$(cat "${CONFIG_FILE}")${RESET}"

# ─── Show loaded extensions ────────────────────────────────────────────────
section "Available Extensions"
echo ""
EXT_SEARCH="${REPO_ROOT}/out/src/extensions"
find "${EXT_SEARCH}" -name 'lib*.so' -type f 2>/dev/null | sort | while read -r so; do
    REL=$(echo "${so}" | sed "s|${REPO_ROOT}/||")
    echo -e "  ${GREEN}●${RESET} ${REL}"
done
echo ""

# ─── Show manifest ─────────────────────────────────────────────────────────
section "Update Manifest"
echo ""
for mf in "${ADUC_SIM_MANIFEST_DIR}"/*.json; do
    echo -e "  ${BLUE}File:${RESET} $(basename "${mf}")"
    echo -e "${DIM}"
    cat "${mf}" | head -40
    echo -e "${RESET}"
done

# ─── Run the agent ─────────────────────────────────────────────────────────
section "Running ADU Gen2 Agent"
echo ""
echo -e "  ${BOLD}Binary:${RESET}  ${AGENT_BINARY}"
echo -e "  ${BOLD}Config:${RESET}  ${CONFIG_FILE}"
echo -e "  ${BOLD}Mode:${RESET}    once (single deployment cycle)"
echo ""
echo -e "${YELLOW}${BOLD}┌─────────────────────────── Agent Output ───────────────────────────────┐${RESET}"

AGENT_EXIT=0
"${AGENT_BINARY}" --config "${CONFIG_FILE}" --once --log-level 6 2>&1 | while IFS= read -r line; do
    echo -e "${DIM}│${RESET} ${line}"
done || AGENT_EXIT=$?

echo -e "${YELLOW}${BOLD}└────────────────────────────────────────────────────────────────────────┘${RESET}"
echo ""

# ─── Show results ──────────────────────────────────────────────────────────
section "Deployment Results"

# Result JSON from simulator
echo ""
echo -e "  ${BLUE}Result directory:${RESET} ${ADUC_SIM_RESULT_DIR}"
if ls "${ADUC_SIM_RESULT_DIR}"/*.json 1>/dev/null 2>&1; then
    for rfile in "${ADUC_SIM_RESULT_DIR}"/*.json; do
        echo -e "  ${BLUE}File:${RESET} $(basename "${rfile}")"
        echo -e "${DIM}"
        cat "${rfile}"
        echo -e "${RESET}"
    done
else
    echo -e "  ${DIM}(no result files yet)${RESET}"
fi

# Installed files
section "Installed Files"
echo ""
INSTALL_TARGET="${DEMO_BASE}/installed"
if [ -d "${INSTALL_TARGET}" ] && [ "$(ls -A "${INSTALL_TARGET}" 2>/dev/null)" ]; then
    find "${INSTALL_TARGET}" -type f | while read -r f; do
        SIZE=$(stat -c%s "${f}" 2>/dev/null || echo "?")
        echo -e "  ${GREEN}✓${RESET} $(basename "${f}")  (${SIZE} bytes)"
    done
else
    echo -e "  ${DIM}(no installed files — install target: ${INSTALL_TARGET})${RESET}"
fi

# Agent log tail
section "Agent Log (last 20 lines)"
echo ""
LOG_FILE="${DEMO_BASE}/adu-agent.log"
if [ -f "${LOG_FILE}" ]; then
    echo -e "${DIM}"
    tail -20 "${LOG_FILE}"
    echo -e "${RESET}"
else
    echo -e "  ${DIM}(no log file)${RESET}"
fi

# ─── Summary ───────────────────────────────────────────────────────────────
echo ""
if [ ${AGENT_EXIT} -eq 0 ]; then
    echo -e "${GREEN}${BOLD}╔══════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${GREEN}${BOLD}║  ✅  Demo completed successfully!                           ║${RESET}"
    echo -e "${GREEN}${BOLD}║                                                             ║${RESET}"
    echo -e "${GREEN}${BOLD}║  What happened:                                             ║${RESET}"
    echo -e "${GREEN}${BOLD}║   1. Agent loaded extensions (simulator, file sideloader,   ║${RESET}"
    echo -e "${GREEN}${BOLD}║      script handler, delta processor)                       ║${RESET}"
    echo -e "${GREEN}${BOLD}║   2. Simulator comm provided the update manifest            ║${RESET}"
    echo -e "${GREEN}${BOLD}║   3. DAG engine resolved step dependencies                  ║${RESET}"
    echo -e "${GREEN}${BOLD}║   4. File sideloader downloaded content via file:// URIs    ║${RESET}"
    echo -e "${GREEN}${BOLD}║   5. Script handler executed install.sh                     ║${RESET}"
    echo -e "${GREEN}${BOLD}║   6. Agent reported v2 protocol result (outcome/origin)     ║${RESET}"
    echo -e "${GREEN}${BOLD}╚══════════════════════════════════════════════════════════════╝${RESET}"
else
    echo -e "${RED}${BOLD}╔══════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${RED}${BOLD}║  ⚠  Agent exited with code: ${AGENT_EXIT}                             ║${RESET}"
    echo -e "${RED}${BOLD}║  Check the log: ${LOG_FILE}${RESET}"
    echo -e "${RED}${BOLD}╚══════════════════════════════════════════════════════════════╝${RESET}"
fi
echo ""
