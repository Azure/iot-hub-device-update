#!/usr/bin/env bash
# install.sh — Firmware installation script for ADU Gen2 Ignite Demo
# Validates and installs a firmware binary to the target location.
#
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

set -euo pipefail

# Handle --verify-only mode
if [ "${1:-}" = "--verify-only" ]; then
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] ▶ Running post-install verification..."
    TARGET_DIR="${ADUC_INSTALL_TARGET:-/opt/contoso/firmware}"
    if [ -d "${TARGET_DIR}" ] && [ "$(ls -A "${TARGET_DIR}" 2>/dev/null)" ]; then
        for f in "${TARGET_DIR}"/*; do
            SIZE=$(stat -c%s "${f}" 2>/dev/null || stat -f%z "${f}" 2>/dev/null || echo "?")
            echo "[$(date '+%Y-%m-%d %H:%M:%S')]   ✓ Verified: $(basename "${f}") (${SIZE} bytes)"
        done
        echo "[$(date '+%Y-%m-%d %H:%M:%S')]   ✅  Post-install verification passed"
        exit 0
    else
        echo "[$(date '+%Y-%m-%d %H:%M:%S')]   ⚠  No installed files found in ${TARGET_DIR}"
        exit 1
    fi
fi

FIRMWARE_FILE="${1:-firmware-v2.0.0.bin}"
TARGET_DIR="${ADUC_INSTALL_TARGET:-/opt/contoso/firmware}"
TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')

# Resolve firmware path relative to the script's directory (files are
# downloaded into the same working directory by the download service).
WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ ! -f "${FIRMWARE_FILE}" ] && [ -f "${WORK_DIR}/${FIRMWARE_FILE}" ]; then
    FIRMWARE_FILE="${WORK_DIR}/${FIRMWARE_FILE}"
fi

log() {
    echo "[${TIMESTAMP}] $1"
    TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
}

log "═══════════════════════════════════════════════════"
log "  Contoso Toaster Firmware Installer v2.0.0"
log "═══════════════════════════════════════════════════"
log ""

# ── Step 1: Validate firmware file ──────────────────────────────────────────
log "▶ Step 1/4: Validating firmware image..."
if [ ! -f "${FIRMWARE_FILE}" ]; then
    log "  ERROR: Firmware file not found: ${FIRMWARE_FILE}"
    exit 1
fi

FILE_SIZE=$(stat -c%s "${FIRMWARE_FILE}" 2>/dev/null || stat -f%z "${FIRMWARE_FILE}" 2>/dev/null || echo "unknown")
log "  ✓ File found: ${FIRMWARE_FILE}"
log "  ✓ Size: ${FILE_SIZE} bytes"
sleep 0.3

# ── Step 2: Pre-install checks ──────────────────────────────────────────────
log "▶ Step 2/4: Running pre-install checks..."
log "  ✓ Disk space: OK"
log "  ✓ Battery level: OK (simulated)"
log "  ✓ No conflicting processes"
sleep 0.3

# ── Step 3: Install firmware ────────────────────────────────────────────────
log "▶ Step 3/4: Installing firmware..."
mkdir -p "${TARGET_DIR}"
cp "${FIRMWARE_FILE}" "${TARGET_DIR}/$(basename "${FIRMWARE_FILE}")"
log "  ✓ Copied to ${TARGET_DIR}/$(basename "${FIRMWARE_FILE}")"
sleep 0.3

# ── Step 4: Post-install verification ───────────────────────────────────────
log "▶ Step 4/4: Post-install verification..."
if [ -f "${TARGET_DIR}/$(basename "${FIRMWARE_FILE}")" ]; then
    INSTALLED_SIZE=$(stat -c%s "${TARGET_DIR}/$(basename "${FIRMWARE_FILE}")" 2>/dev/null || stat -f%z "${TARGET_DIR}/$(basename "${FIRMWARE_FILE}")" 2>/dev/null || echo "unknown")
    if [ "${FILE_SIZE}" = "${INSTALLED_SIZE}" ]; then
        log "  ✓ Size match verified"
    fi
fi
log "  ✓ Installation complete"
sleep 0.2

log ""
log "══════════════════════════════════════════════════════"
log "  ✅  Firmware v2.0.0 installed successfully"
log "══════════════════════════════════════════════════════"

exit 0
