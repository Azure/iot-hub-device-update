#!/bin/bash
# ADU Gen2 Agent - Full Feature Demo for Ignite
# Demonstrates: DAG-based step dependencies, multiple handler types, rollback
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
AGENT_BIN="${SCRIPT_DIR}/../../out/bin/adu_gen2_agent"
MOCK_SERVER="${SCRIPT_DIR}/../mock_adu_server/server.py"
MOCK_IOTHUB="${SCRIPT_DIR}/../mock_iothub/server.py"
MANIFEST="${1:-${SCRIPT_DIR}/manifests/multi_step_manifest.json}"
PAYLOADS="${SCRIPT_DIR}/payloads"
CONFIG="${SCRIPT_DIR}/adu-agent-demo.conf"

echo "╔══════════════════════════════════════════════════════════╗"
echo "║     ADU Gen2 Agent - Ignite Conference Demo             ║"
echo "║     DAG-based Orchestration + Extension Framework       ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""
echo "Manifest: $(basename $MANIFEST)"
echo "Agent:    $AGENT_BIN"
echo ""

# Check prerequisites
if [ ! -f "$AGENT_BIN" ]; then
    echo "ERROR: Agent binary not found at $AGENT_BIN"
    echo "       Run: cd ../../ && ./scripts/build.sh"
    exit 1
fi

# Start mock ADU server
echo "▶ Starting mock ADU server..."
python3 "$MOCK_SERVER" \
    --port 8080 \
    --manifest-file "$MANIFEST" \
    --content-dir "$PAYLOADS" \
    --verbose &
SERVER_PID=$!
sleep 1

# Verify server is up
if ! curl -s http://localhost:8080/health > /dev/null; then
    echo "ERROR: Mock server failed to start"
    kill $SERVER_PID 2>/dev/null
    exit 1
fi
echo "  ✓ Mock server running (PID: $SERVER_PID)"
echo ""

# Run the agent
echo "▶ Running ADU Gen2 Agent..."
echo "  Config: $CONFIG"
echo "  Mode: --once (single poll cycle)"
echo ""
echo "───────────────── Agent Output ─────────────────"
"$AGENT_BIN" --config "$CONFIG" --once --log-level 6 2>&1 | while IFS= read -r line; do
    echo "  $line"
done
AGENT_EXIT=$?
echo "───────────────── End Output ───────────────────"
echo ""

# Report results
if [ $AGENT_EXIT -eq 0 ]; then
    echo "✅ Demo completed successfully!"
else
    echo "⚠️  Agent exited with code: $AGENT_EXIT"
fi

# Show server logs summary
echo ""
echo "▶ Server received deployments:"
curl -s http://localhost:8080/admin/status 2>/dev/null | python3 -m json.tool 2>/dev/null || echo "  (server already stopped)"

# Cleanup
echo ""
echo "▶ Cleaning up..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
echo "  ✓ Mock server stopped"
echo ""
echo "Demo complete!"
