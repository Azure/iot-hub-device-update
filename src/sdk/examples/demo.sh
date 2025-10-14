#!/bin/bash
# Demo script showing ADU SDK example usage

set -e

EXAMPLES_DIR="$(dirname "$0")"
cd "$EXAMPLES_DIR"

echo "ADU SDK Examples Demo"
echo "===================="
echo

# Check if examples are built
if [ ! -f "simple_status_check" ]; then
    echo "Building examples first..."
    make all
    echo
fi

echo "1. Basic Status Check"
echo "--------------------"
echo "$ ./simple_status_check"
./simple_status_check
echo

echo "2. Verbose Status Check"
echo "----------------------"
echo "$ ./simple_status_check --verbose"
./simple_status_check --verbose
echo

echo "3. Quiet Status Check (exit code only)"
echo "--------------------------------------"
echo "$ ./simple_status_check --quiet; echo \"Exit code: \$?\""
./simple_status_check --quiet
echo "Exit code: $?"
echo

echo "4. Status Monitor (3 iterations)"
echo "--------------------------------"
echo "$ timeout 3 ./status_monitor --interval 1 || true"
timeout 3 ./status_monitor --interval 1 || true
echo

echo "5. JSON Format Output"
echo "--------------------"
echo "$ ./status_monitor --format json --interval 1 --no-timestamps | head -3"
timeout 3 ./status_monitor --format json --interval 1 --no-timestamps | head -3 || true
echo

echo "Demo complete!"
echo
echo "Available examples:"
echo "  simple_status_check  - Basic status checking with error handling"
echo "  status_monitor       - Continuous monitoring with multiple formats"
echo
echo "Use --help with any example for detailed usage information."
