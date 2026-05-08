#!/usr/bin/env bash
# cleanup_demo.sh — Remove all ADU Gen2 Ignite demo temp files.
#
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEMO_BASE="${SCRIPT_DIR}/.demo-env"

BOLD='\033[1m'; GREEN='\033[0;32m'; DIM='\033[2m'; RESET='\033[0m'

echo -e "${BOLD}ADU Gen2 Ignite Demo — Cleanup${RESET}"
echo ""

if [ -d "${DEMO_BASE}" ]; then
    echo -e "  Removing ${DIM}${DEMO_BASE}${RESET}"
    rm -rf "${DEMO_BASE}"
    echo -e "  ${GREEN}✓${RESET} Demo environment cleaned up"
else
    echo -e "  ${DIM}Nothing to clean — demo environment not found${RESET}"
fi

# Unset env vars if they are set
unset ADUC_SIM_MANIFEST_DIR 2>/dev/null || true
unset ADUC_SIM_CONTENT_DIR 2>/dev/null || true
unset ADUC_SIM_RESULT_DIR 2>/dev/null || true
unset ADUC_INSTALL_TARGET 2>/dev/null || true

echo ""
echo -e "  ${GREEN}✓${RESET} Done"
echo ""
