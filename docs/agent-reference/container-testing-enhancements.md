# Container Testing Enhancements for API Service

## Overview

This document describes the comprehensive container testing improvements implemented to address apisvc test failures in containerized environments. The enhancements provide better debugging, timeout protection, and container-specific testing capabilities.

## Problem Statement

The apisvc crossproc tests were experiencing non-responsive behavior when running as part of container builds in CI/CD pipelines. This was manifesting as:

- Tests hanging indefinitely without output
- FIFO operation failures in container environments
- Limited debugging information in CI logs
- No timeout protection for hanging tests

## Solution Components

### 1. Enhanced Debugging Script (`scripts/test-apisvc-debug.sh`)

**Purpose**: Comprehensive debugging tool for apisvc tests with container awareness

**Key Features**:
- Environment analysis (container detection, filesystem permissions, resource availability)
- FIFO operation testing in multiple directories (`/tmp`, `/var/tmp`, `/dev/shm`)
- Process monitoring with timeout protection
- Stress testing capabilities
- Detailed logging and error reporting
- `strace` integration for low-level debugging

**Usage**:
```bash
# Basic debugging
./scripts/test-apisvc-debug.sh

# Container-specific debugging with verbose output
./scripts/test-apisvc-debug.sh container-debug --verbose

# Stress testing
./scripts/test-apisvc-debug.sh stress-test --count 10
```

### 2. Container-Specific Test Suite (`src/agent/api/tests/apisvc_container_unit_tests.cpp`)

**Purpose**: Specialized test suite designed for container environments

**Key Features**:
- Dynamic FIFO directory selection (prefers `/tmp`, `/var/tmp`, `/dev/shm`)
- Extended timeouts for container-specific latencies
- Enhanced error reporting and debugging output
- Container environment detection and validation
- Alternative IPC location testing
- Stress testing with container constraints

**Test Scenarios**:
- Container environment validation
- FIFO directory selection and testing
- Extended timeout handling for `GETSTATE` operations
- Stress testing with reduced iteration counts for container limits

### 3. Container Test Runner (`scripts/run-container-tests.sh`)

**Purpose**: Specialized test execution environment for containers

**Key Features**:
- Container environment detection and analysis
- FIFO operation pre-testing
- Process monitoring with timeout protection
- Resource usage monitoring (CPU/memory)
- Comprehensive cleanup procedures
- Optional strace integration for deep debugging

**Options**:
```bash
# Basic container testing
./scripts/run-container-tests.sh

# Verbose mode with strace
./scripts/run-container-tests.sh --verbose

# Custom timeout and build directory
./scripts/run-container-tests.sh --timeout 180 --build-dir /path/to/build
```

### 4. Enhanced CI/CD Pipeline (`.github/workflows/docker-build.yml`)

**Purpose**: Improved GitHub Actions workflow with container debugging

**Enhancements**:
- Container environment information collection
- Multiple testing approaches (container-specific, fallback debugging, standard)
- Timeout protection for all test phases
- Enhanced error reporting and artifact collection
- Process monitoring and resource usage reporting

**Test Flow**:
1. Environment analysis and reporting
2. Container-specific apisvc testing
3. Fallback debugging if container tests fail
4. Full test suite execution with timeout protection
5. Comprehensive error reporting and cleanup

## Implementation Details

### FIFO Directory Selection Algorithm

The container tests implement intelligent FIFO directory selection:

```cpp
std::vector<std::string> candidates = {
    "/tmp",           // Standard temporary directory
    "/var/tmp",       // Alternative temporary directory
    "/dev/shm",       // Shared memory filesystem (often faster)
    "$HOME/tmp"       // User-specific temporary directory
};
```

For each candidate:
1. Check directory existence and write permissions
2. Test FIFO creation and operation
3. Verify proper cleanup
4. Select first working directory

### Timeout Protection Strategy

Multiple layers of timeout protection:

1. **Individual Test Timeouts**: Each test operation has specific timeouts
2. **Process Monitoring**: Background monitoring with progress reporting
3. **CI/CD Timeouts**: GitHub Actions job-level timeout protection
4. **Graceful Degradation**: Tests continue even if some components fail

### Container-Specific Optimizations

#### Extended Timeouts
- Standard timeout: 5 seconds
- Container timeout: 15 seconds
- CI/CD timeout: 120 seconds total

#### Resource Constraints
- Reduced stress test iterations (3 vs 10)
- More frequent progress reporting
- Enhanced cleanup procedures

#### Alternative IPC Locations
- Primary: `/tmp` (most reliable)
- Secondary: `/var/tmp` (persistent across reboots)
- Tertiary: `/dev/shm` (memory-based, fastest)
- Fallback: `$HOME/tmp` (user-specific)

## Usage Guidelines

### For Local Development

1. **Standard Testing**:
   ```bash
   cd build
   make apisvc_unit_tests
   ./bin/apisvc_unit_tests
   ```

2. **Container Testing**:
   ```bash
   cd build
   make apisvc_container_unit_tests
   ../scripts/run-container-tests.sh
   ```

3. **Debugging Issues**:
   ```bash
   ./scripts/test-apisvc-debug.sh container-debug --verbose
   ```

### For CI/CD Environments

The enhanced GitHub Actions workflow automatically:
1. Detects container environment
2. Runs appropriate tests with timeout protection
3. Provides detailed debugging output
4. Continues with other tests even if apisvc tests fail

### For Manual Container Testing

```bash
# In a Docker container
docker run -it --rm debian:12 bash

# Install dependencies and build
apt-get update && apt-get install -y git cmake build-essential
git clone <repository>
cd <repository>
scripts/install-deps.sh --install-aduc-deps --install-cmake
scripts/build.sh --clean --build-unit-tests

# Run container-specific tests
scripts/run-container-tests.sh --verbose
```

## Troubleshooting

### Common Container Issues

1. **FIFO Creation Failures**:
   - Check directory permissions
   - Verify filesystem support for named pipes
   - Try alternative directories (`/var/tmp`, `/dev/shm`)

2. **Test Timeouts**:
   - Increase timeout values in scripts
   - Check container resource limits
   - Monitor for process deadlocks

3. **Permission Denied Errors**:
   - Run tests as root in container
   - Check directory write permissions
   - Verify container security policies

### Debugging Commands

```bash
# Check FIFO support
mkfifo /tmp/test_fifo && ls -la /tmp/test_fifo && rm /tmp/test_fifo

# Monitor test processes
ps aux | grep apisvc

# Check for leftover FIFO files
find /tmp -name "*fifo*" -type p

# Check container environment
env | grep -E "(CONTAINER|DOCKER)"
cat /proc/1/cgroup
```

## Files Modified/Created

### New Files
- `src/agent/api/tests/apisvc_container_unit_tests.cpp`
- `scripts/test-apisvc-debug.sh`
- `scripts/run-container-tests.sh`

### Modified Files
- `src/agent/api/tests/apisvc_unit_tests.cpp` (race condition fixes)
- `src/agent/api/tests/CMakeLists.txt` (added container tests)
- `src/agent/api/src/apisvc.c` (added readiness flag)
- `.github/workflows/docker-build.yml` (enhanced debugging)

### Documentation
- `docs/agent-reference/apisvc-testing-guide.md`
- `docs/agent-reference/container-testing-best-practices.md`
- Various other documentation updates

## Benefits

1. **Improved Reliability**: Tests now handle container-specific issues gracefully
2. **Better Debugging**: Comprehensive logging and diagnostic information
3. **Timeout Protection**: No more hanging CI/CD jobs
4. **Container Awareness**: Tests adapt to container environment constraints
5. **Graceful Degradation**: System continues working even when some tests fail
6. **Enhanced Monitoring**: Real-time progress reporting and resource usage tracking

## Future Enhancements

1. **Alternative IPC Mechanisms**: Consider Unix domain sockets as FIFO alternatives
2. **Performance Optimization**: Optimize tests for container resource constraints
3. **Cross-Platform Testing**: Extend container support to Windows containers
4. **Automated Debugging**: Auto-enable verbose mode when tests fail
5. **Test Parallelization**: Safely run multiple container tests simultaneously