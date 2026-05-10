#!/usr/bin/env bash
# adu_status_check.sh — Query ADU Gen2 Agent status via Local API.
#
# Connects to the agent's Unix socket and displays:
#   - Current agent status
#   - Loaded extensions
#   - Component inventory
#
# Usage: ./adu_status_check.sh [--socket <path>]
#
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

set -euo pipefail

SOCKET_PATH="/var/run/adu-agent/local.sock"
AGENT_API="http://localhost/api/v1"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --socket) SOCKET_PATH="$2"; shift 2 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

# ─── Helpers ──────────────────────────────────────────────────────────────────

print_header() {
    local title="$1"
    echo ""
    echo "┌─$(printf '─%.0s' $(seq 1 ${#title}))─┐"
    echo "│ ${title} │"
    echo "└─$(printf '─%.0s' $(seq 1 ${#title}))─┘"
}

print_table() {
    # Reads JSON array and prints as table
    # Usage: echo "$json" | print_table "field1" "field2" ...
    column -t -s $'\t'
}

curl_socket() {
    local endpoint="$1"
    curl -s --unix-socket "${SOCKET_PATH}" "${AGENT_API}${endpoint}" 2>/dev/null
}

# ─── Check connectivity ──────────────────────────────────────────────────────

if [[ ! -S "${SOCKET_PATH}" ]]; then
    echo "ERROR: Agent socket not found at ${SOCKET_PATH}"
    echo ""
    echo "Ensure the ADU Gen2 Agent is running."
    echo "  Socket path: ${SOCKET_PATH}"
    echo ""
    echo "Try:"
    echo "  ./run_demo.sh   (start the agent)"
    echo "  --socket <path> (specify different socket path)"
    exit 1
fi

# ─── Query Status ────────────────────────────────────────────────────────────

print_header "Agent Status"

STATUS_JSON=$(curl_socket "/status")
if [[ -z "${STATUS_JSON}" ]]; then
    echo "  (no response — agent may not support Local API yet)"
else
    echo "${STATUS_JSON}" | python3 -c "
import sys, json
try:
    d = json.load(sys.stdin)
    print(f\"  State:       {d.get('state', 'unknown')}\")
    print(f\"  Version:     {d.get('version', 'unknown')}\")
    print(f\"  Device ID:   {d.get('deviceId', 'unknown')}\")
    print(f\"  Uptime:      {d.get('uptimeSec', 0)}s\")
    print(f\"  Workflow:    {d.get('activeWorkflow', 'none')}\")
except:
    print('  (unable to parse response)')
" 2>/dev/null || echo "  ${STATUS_JSON}"
fi

# ─── Query Extensions ────────────────────────────────────────────────────────

print_header "Loaded Extensions"

EXT_JSON=$(curl_socket "/extensions")
if [[ -z "${EXT_JSON}" ]]; then
    echo "  (no response)"
else
    echo "${EXT_JSON}" | python3 -c "
import sys, json
try:
    exts = json.load(sys.stdin)
    if not exts:
        print('  (none loaded)')
        sys.exit(0)
    # Header
    print(f\"  {'ID':<25} {'Type':<18} {'Version':<10} {'Capabilities'}\")
    print(f\"  {'─'*25} {'─'*18} {'─'*10} {'─'*30}\")
    for e in exts:
        eid = e.get('id', '?')
        etype = e.get('type', '?')
        ever = e.get('version', '?')
        caps = ', '.join(e.get('capabilities', []) or ['(none)'])
        print(f\"  {eid:<25} {etype:<18} {ever:<10} {caps}\")
except:
    print('  (unable to parse response)')
" 2>/dev/null || echo "  ${EXT_JSON}"
fi

# ─── Query Components ────────────────────────────────────────────────────────

print_header "Component Inventory"

COMP_JSON=$(curl_socket "/components")
if [[ -z "${COMP_JSON}" ]]; then
    echo "  (no response)"
else
    echo "${COMP_JSON}" | python3 -c "
import sys, json
try:
    comps = json.load(sys.stdin)
    if not comps:
        print('  (no components)')
        sys.exit(0)
    print(f\"  {'ID':<20} {'Group':<20} {'Name':<25} {'Version'}\")
    print(f\"  {'─'*20} {'─'*20} {'─'*25} {'─'*15}\")
    for c in comps:
        cid = c.get('id', '?')
        grp = c.get('group', '?')
        name = c.get('name', '?')
        ver = c.get('installedVersion', '?')
        print(f\"  {cid:<20} {grp:<20} {name:<25} {ver}\")
except:
    print('  (unable to parse response)')
" 2>/dev/null || echo "  ${COMP_JSON}"
fi

echo ""
