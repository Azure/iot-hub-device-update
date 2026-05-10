#!/usr/bin/env bash
# run_demo.sh — Interactive demo of ADU Gen2 Agent.
#
# Starts the mock server and agent, then lets you observe the output.
# Press Ctrl+C to stop everything.
#
# Usage: ./run_demo.sh [--build-dir <path>]
#
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${REPO_ROOT}/out/build"
MOCK_SERVER_DIR="${REPO_ROOT}/tools/mock_adu_server"
MOCK_SERVER_PORT=8199
AGENT_BINARY="${BUILD_DIR}/src/agent/adu_gen2_agent"
CONFIG_FILE="${SCRIPT_DIR}/adu-agent.conf"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir) BUILD_DIR="$2"; AGENT_BINARY="${BUILD_DIR}/src/agent/adu_gen2_agent"; shift 2 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

cleanup() {
    echo ""
    echo "=== Stopping demo ==="
    [[ -n "${AGENT_PID:-}" ]] && kill "$AGENT_PID" 2>/dev/null || true
    [[ -n "${SERVER_PID:-}" ]] && kill "$SERVER_PID" 2>/dev/null || true
    echo "Done."
}
trap cleanup EXIT INT TERM

echo "╔═══════════════════════════════════════════╗"
echo "║   ADU Gen2 Agent — Interactive Demo       ║"
echo "╚═══════════════════════════════════════════╝"
echo ""
echo "Build dir:     ${BUILD_DIR}"
echo "Config:        ${CONFIG_FILE}"
echo "Mock server:   http://127.0.0.1:${MOCK_SERVER_PORT}"
echo ""

# ─── Start mock server ───────────────────────────────────────────────────────
echo ">>> Starting mock ADU server..."
if [[ -f "${MOCK_SERVER_DIR}/server.py" ]]; then
    python3 "${MOCK_SERVER_DIR}/server.py" --port "${MOCK_SERVER_PORT}" &
    SERVER_PID=$!
    sleep 1
    echo "    Mock server running (PID: ${SERVER_PID})"
else
    echo "    WARNING: Mock server not found — skipping"
fi

echo ""

# ─── Start agent ─────────────────────────────────────────────────────────────
echo ">>> Starting ADU Gen2 Agent..."
if [[ -x "${AGENT_BINARY}" ]]; then
    echo "    Agent binary: ${AGENT_BINARY}"
    echo "    Config: ${CONFIG_FILE}"
    echo ""
    echo "═══════════════════════════════════════════════"
    echo "  Watch the agent output below."
    echo "  Press Ctrl+C to stop the demo."
    echo "═══════════════════════════════════════════════"
    echo ""
    "${AGENT_BINARY}" --config "${CONFIG_FILE}" &
    AGENT_PID=$!
    wait "$AGENT_PID" || true
else
    echo "    Agent binary not found at: ${AGENT_BINARY}"
    echo "    Build the project first:"
    echo "      cmake --build ${BUILD_DIR}"
    echo ""
    echo "    Showing mock server activity instead..."
    echo "    Press Ctrl+C to stop."
    echo ""
    # Keep running so user can interact with mock server
    wait "${SERVER_PID:-}" 2>/dev/null || sleep 3600
fi
