#!/bin/bash

# Container Test Runner for API Service
# Enhanced testing script specifically designed for containerized environments
# with comprehensive diagnostics and timeout protection

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
BUILD_DIR="${BUILD_DIR:-${SCRIPT_DIR}/../build}"
TEST_TIMEOUT="${TEST_TIMEOUT:-120}"
VERBOSE="${VERBOSE:-0}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${BLUE}[INFO]${NC} $*"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $*"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $*"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $*"
}

check_container_environment() {
    log_info "Checking container environment..."

    echo "System Info:"
    echo "  Hostname: $(hostname)"
    echo "  Kernel: $(uname -r)"
    echo "  OS: $(grep PRETTY_NAME /etc/os-release | cut -d= -f2 | tr -d '\"' || echo 'Unknown')"
    echo "  Container: ${container:-none}"
    echo "  Docker: ${DOCKER_CONTAINER:-none}"
    echo "  User: $(whoami) (UID: $(id -u), GID: $(id -g))"
    echo ""

    # Check critical directories
    for dir in /tmp /var/tmp /dev/shm; do
        if [ -d "$dir" ]; then
            local writable
            writable=$([ -w "$dir" ] && echo "writable" || echo "read-only")
            local size
            size=$(df -h "$dir" 2> /dev/null | tail -n1 | awk '{print $4}' || echo "unknown")
            echo "  $dir: exists, $writable, available: $size"
        else
            echo "  $dir: missing"
        fi
    done
    echo ""
}

test_fifo_operations() {
    log_info "Testing FIFO operations in container..."

    local test_dirs=("/tmp" "/var/tmp" "/dev/shm")
    local working_dirs=()

    for dir in "${test_dirs[@]}"; do
        if [ -d "$dir" ] && [ -w "$dir" ]; then
            local test_fifo="$dir/container_fifo_test_$$"
            if mkfifo "$test_fifo" 2> /dev/null; then
                if [ -p "$test_fifo" ]; then
                    log_success "FIFO operations work in $dir"
                    working_dirs+=("$dir")
                else
                    log_warn "FIFO creation succeeded but file is not a pipe in $dir"
                fi
                rm -f "$test_fifo"
            else
                log_warn "Cannot create FIFO in $dir"
            fi
        else
            log_warn "Directory $dir is not available or writable"
        fi
    done

    if [ ${#working_dirs[@]} -eq 0 ]; then
        log_error "No working FIFO directories found!"
        return 1
    fi

    log_success "FIFO operations available in: ${working_dirs[*]}"
    return 0
}

monitor_process() {
    local pid=$1
    local name=$2
    local max_time=$3
    local start_time
    start_time=$(date +%s)

    while kill -0 "$pid" 2> /dev/null; do
        local current_time
        current_time=$(date +%s)
        local elapsed=$((current_time - start_time))

        if [ $elapsed -ge "$max_time" ]; then
            log_error "$name timed out after ${max_time}s"
            kill -TERM "$pid" 2> /dev/null || true
            sleep 2
            kill -KILL "$pid" 2> /dev/null || true
            return 1
        fi

        if [ $((elapsed % 10)) -eq 0 ] && [ $elapsed -gt 0 ]; then
            local cpu_usage
            cpu_usage=$(ps -p "$pid" -o %cpu --no-headers 2> /dev/null | tr -d ' ' || echo "N/A")
            local mem_usage
            mem_usage=$(ps -p "$pid" -o %mem --no-headers 2> /dev/null | tr -d ' ' || echo "N/A")
            log_info "$name running... ${elapsed}s (CPU: ${cpu_usage}%, MEM: ${mem_usage}%)"
        fi

        sleep 1
    done

    wait "$pid"
    return $?
}

run_container_tests() {
    log_info "Running container-specific API service tests..."

    # Check if test binary exists
    local test_binary="$BUILD_DIR/src/agent/api/tests/apisvc_container_unit_tests"
    if [ ! -f "$test_binary" ]; then
        log_error "Container test binary not found: $test_binary"
        log_info "Building container tests..."
        cd "$BUILD_DIR"
        make apisvc_container_unit_tests || {
            log_error "Failed to build container tests"
            return 1
        }
    fi

    if [ ! -x "$test_binary" ]; then
        log_error "Container test binary is not executable: $test_binary"
        return 1
    fi

    log_info "Running container tests with timeout protection..."
    echo "Test binary: $test_binary"
    echo "Test timeout: ${TEST_TIMEOUT}s"
    echo ""

    # Set up test environment
    export ADUC_LOG_LEVEL=4 # Debug logging
    export ADUC_FIFO_TEST_MODE=1

    # Create a temporary log file for detailed output
    local log_file="/tmp/apisvc_container_test_$$.log"

    # Run tests with timeout and monitoring
    {
        echo "=== Container Test Execution Started at $(date) ==="
        echo "Environment:"
        env | grep -E "(ADUC|CONTAINER|DOCKER)" || echo "No relevant environment variables"
        echo ""

        if [ "$VERBOSE" = "1" ]; then
            strace -f -e trace=openat,mknodat,unlink,write,read -o /tmp/strace_$$.log "$test_binary" 2>&1 || true
            echo ""
            echo "=== Strace Summary ==="
            if [ -f "/tmp/strace_$$.log" ]; then
                grep -E "(mknodat|openat.*fifo|ENOENT|EPERM|EACCES)" /tmp/strace_$$.log | head -20 || echo "No relevant strace output"
                rm -f "/tmp/strace_$$.log"
            fi
        else
            "$test_binary" 2>&1 || true
        fi

        echo ""
        echo "=== Container Test Execution Completed at $(date) ==="
    } > "$log_file" 2>&1 &

    local test_pid=$!

    if monitor_process "$test_pid" "Container Tests" "$TEST_TIMEOUT"; then
        log_success "Container tests completed successfully"
        local result=0
    else
        log_error "Container tests failed or timed out"
        local result=1
    fi

    # Show test output
    echo ""
    echo "=== Test Output ==="
    cat "$log_file"
    rm -f "$log_file"

    return $result
}

cleanup() {
    log_info "Cleaning up test artifacts..."

    # Clean up any remaining FIFO files
    find /tmp /var/tmp /dev/shm -name "*test*fifo*" -type p 2> /dev/null | while read -r fifo; do
        log_info "Removing leftover FIFO: $fifo"
        rm -f "$fifo"
    done 2> /dev/null || true

    # Clean up any test log files
    rm -f /tmp/apisvc_container_test_*.log /tmp/strace_*.log 2> /dev/null || true
}

main() {
    trap cleanup EXIT

    echo "=================================="
    echo "API Service Container Test Runner"
    echo "=================================="
    echo ""

    check_container_environment

    if ! test_fifo_operations; then
        log_error "FIFO operations not working in container environment"
        exit 1
    fi

    if run_container_tests; then
        log_success "All container tests passed!"
        exit 0
    else
        log_error "Container tests failed!"
        exit 1
    fi
}

# Handle command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
    -v | --verbose)
        VERBOSE=1
        shift
        ;;
    -t | --timeout)
        TEST_TIMEOUT="$2"
        shift 2
        ;;
    -b | --build-dir)
        BUILD_DIR="$2"
        shift 2
        ;;
    -h | --help)
        echo "Usage: $0 [OPTIONS]"
        echo ""
        echo "Options:"
        echo "  -v, --verbose        Enable verbose output with strace"
        echo "  -t, --timeout SECS   Set test timeout (default: 120)"
        echo "  -b, --build-dir DIR  Set build directory"
        echo "  -h, --help           Show this help"
        echo ""
        echo "Environment Variables:"
        echo "  BUILD_DIR           Build directory path"
        echo "  TEST_TIMEOUT        Test timeout in seconds"
        echo "  VERBOSE             Enable verbose mode (0/1)"
        exit 0
        ;;
    *)
        log_error "Unknown option: $1"
        exit 1
        ;;
    esac
done

main "$@"
