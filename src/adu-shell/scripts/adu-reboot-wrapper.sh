#!/bin/bash
# ADU Reboot Wrapper - waits for agent to prepare for reboot
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

LOCK_FILE="/var/run/adu-agent-reboot.lock"
DEFAULT_TIMEOUT=60
INTERVAL=1

# Get timeout from config or use default
TIMEOUT=${ADU_REBOOT_TIMEOUT:-$DEFAULT_TIMEOUT}

elapsed=0

echo "ADU Reboot Wrapper: Waiting for agent to prepare (timeout: ${TIMEOUT}s)..."

while [ -f "$LOCK_FILE" ] && [ $elapsed -lt "$TIMEOUT" ]; do
    # Verify PID is still alive if lock contains PID
    if [ -r "$LOCK_FILE" ]; then
        pid=$(cat "$LOCK_FILE" 2>/dev/null)
        if [ -n "$pid" ] && [ "$pid" -gt 0 ]; then
            if ! kill -0 "$pid" 2>/dev/null; then
                echo "ADU Reboot Wrapper: Agent process $pid died, removing stale lock"
                rm -f "$LOCK_FILE"
                break
            fi
        fi
    fi

    sleep $INTERVAL
    elapsed=$((elapsed + INTERVAL))
done

# Cleanup any remaining lock
rm -f "$LOCK_FILE" 2>/dev/null

if [ $elapsed -ge "$TIMEOUT" ]; then
    echo "ADU Reboot Wrapper: Timeout reached after ${elapsed}s, forcing reboot"
else
    echo "ADU Reboot Wrapper: Agent ready after ${elapsed}s, proceeding with reboot"
fi

# Perform actual reboot
exec /sbin/reboot "$@"
