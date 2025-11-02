#!/bin/bash
# Enhanced apisvc test script with container debugging
# Usage: ./test-apisvc-debug.sh [test_name]

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Enhanced apisvc Test Debugging ===${NC}"

# Check environment
echo -e "${YELLOW}Environment Information:${NC}"
echo "Container: ${CONTAINER:-"Not detected"}"
echo "User: $(whoami) (UID: $(id -u), GID: $(id -g))"
echo "Working directory: $(pwd)"
echo "Filesystem info for /tmp:"
df -h /tmp || echo "  /tmp not available"
find /tmp -maxdepth 1 -type f | head -10 || echo "  Cannot list /tmp"

# Check for test binary
if [ ! -f "./bin/apisvc_unit_tests" ]; then
    echo -e "${RED}ERROR: apisvc_unit_tests binary not found in ./bin/${NC}"
    echo "Current directory contents:"
    ls -la .
    echo "bin/ directory contents:"
    ls -la bin/ 2> /dev/null || echo "  bin/ directory not found"
    exit 1
fi

echo -e "${YELLOW}Binary Information:${NC}"
file ./bin/apisvc_unit_tests
ldd ./bin/apisvc_unit_tests | head -10

# Test FIFO creation in /tmp
echo -e "${YELLOW}Testing FIFO Operations:${NC}"
TEST_FIFO="/tmp/test_container_fifo_$$"
if mkfifo "$TEST_FIFO" 2> /dev/null; then
    echo -e "${GREEN}✓ FIFO creation successful${NC}"
    rm -f "$TEST_FIFO"
else
    echo -e "${RED}✗ FIFO creation failed${NC}"
    echo "Trying alternative locations..."

    # Try different locations
    for dir in "/var/tmp" "/dev/shm" "$(pwd)/tmp"; do
        if [ -d "$dir" ] && [ -w "$dir" ]; then
            TEST_FIFO="$dir/test_container_fifo_$$"
            if mkfifo "$TEST_FIFO" 2> /dev/null; then
                echo -e "${GREEN}✓ FIFO creation successful in $dir${NC}"
                rm -f "$TEST_FIFO"
                export ADU_TEST_TMP_DIR="$dir"
                break
            fi
        fi
    done
fi

# Function to run test with timeout and detailed logging
run_apisvc_test() {
    local timeout_seconds=${1:-30}
    local test_args=${2:-""}

    echo -e "${YELLOW}Running apisvc test (timeout: ${timeout_seconds}s)${NC}"
    echo "Command: timeout ${timeout_seconds}s ./bin/apisvc_unit_tests $test_args"
    echo "Start time: $(date)"

    # Create a temporary log file
    local log_file="/tmp/apisvc_test_$$.log"

    # Run test with timeout, capturing all output
    if timeout "$timeout_seconds" stdbuf -oL -eL ./bin/apisvc_unit_tests "$test_args" 2>&1 | tee "$log_file"; then
        echo -e "${GREEN}✓ Test completed successfully${NC}"
        local exit_code=0
    else
        local exit_code=$?
        echo -e "${RED}✗ Test failed or timed out (exit code: $exit_code)${NC}"

        # Analyze the failure
        echo -e "${YELLOW}Failure Analysis:${NC}"
        if [ $exit_code -eq 124 ]; then
            echo "  Test timed out after ${timeout_seconds} seconds"
            echo "  This suggests the test is hanging, likely in FIFO operations"
        fi

        # Check what processes might be hanging
        echo -e "${YELLOW}Process Information:${NC}"
        pgrep -a apisvc || echo "  No apisvc processes found"

        # Check for leftover FIFOs
        echo -e "${YELLOW}Leftover FIFO files:${NC}"
        find /tmp /var/tmp -name "*fifo*" -type p 2> /dev/null || echo "  No FIFO files found"

        # Show last few lines of output
        echo -e "${YELLOW}Last 20 lines of test output:${NC}"
        tail -20 "$log_file" 2> /dev/null || echo "  No log file available"
    fi

    echo "End time: $(date)"
    rm -f "$log_file"
    return $exit_code
}

# Function to run stress test
run_stress_test() {
    local num_tests=${1:-3}
    local timeout_per_test=${2:-15}

    echo -e "${YELLOW}Running stress test: $num_tests parallel tests${NC}"

    local pids=()
    local exit_codes=()

    # Start parallel tests
    for i in $(seq 1 "$num_tests"); do
        echo "Starting test $i..."
        (
            echo "Stress test $i starting at $(date)"
            timeout "$timeout_per_test" ./bin/apisvc_unit_tests --reporter=quiet
            echo "Stress test $i finished at $(date) with exit code $?"
        ) &
        pids+=("$!")
    done

    # Wait for all tests and collect exit codes
    local failed=0
    for i in "${!pids[@]}"; do
        if wait "${pids[$i]}"; then
            echo -e "${GREEN}✓ Stress test $((i + 1)) passed${NC}"
            exit_codes+=("0")
        else
            local code=$?
            echo -e "${RED}✗ Stress test $((i + 1)) failed (exit code: $code)${NC}"
            exit_codes+=("$code")
            failed=1
        fi
    done

    if [ $failed -eq 0 ]; then
        echo -e "${GREEN}✓ All stress tests passed${NC}"
        return 0
    else
        echo -e "${RED}✗ Some stress tests failed${NC}"
        return 1
    fi
}

# Main execution
case "${1:-single}" in
"single")
    run_apisvc_test 30 "--reporter=verbose"
    ;;
"quick")
    run_apisvc_test 10 "--reporter=quiet"
    ;;
"stress")
    run_stress_test 3 15
    ;;
"heavy-stress")
    run_stress_test 5 20
    ;;
"container-debug")
    echo -e "${YELLOW}Running comprehensive container debugging${NC}"

    # Test basic functionality first
    echo -e "${BLUE}Step 1: Basic test${NC}"
    if run_apisvc_test 15 "--reporter=quiet"; then
        echo -e "${GREEN}Basic test passed${NC}"
    else
        echo -e "${RED}Basic test failed - investigating${NC}"

        # Try with strace if available
        if command -v strace > /dev/null 2>&1; then
            echo -e "${YELLOW}Running with strace for debugging${NC}"
            timeout 10s strace -f -e trace=openat,mkfifo,pipe,write,read,poll,select -o /tmp/strace.log ./bin/apisvc_unit_tests --reporter=quiet 2>&1 || true
            echo "strace output (last 50 lines):"
            tail -50 /tmp/strace.log 2> /dev/null || echo "No strace output"
        fi

        exit 1
    fi

    # Test stress scenario
    echo -e "${BLUE}Step 2: Stress test${NC}"
    run_stress_test 2 10
    ;;
*)
    echo "Usage: $0 [single|quick|stress|heavy-stress|container-debug]"
    echo "  single: Run single test with verbose output (default)"
    echo "  quick: Run single test with minimal output"
    echo "  stress: Run 3 parallel tests"
    echo "  heavy-stress: Run 5 parallel tests"
    echo "  container-debug: Comprehensive debugging for container environments"
    exit 1
    ;;
esac
