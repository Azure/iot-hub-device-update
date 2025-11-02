# How to Test the API Service

This guide covers testing the Device Update for IoT Hub API service, with special focus on preventing and detecting race conditions in cross-process communication.

## Overview

The API service (`apisvc`) provides cross-process communication between client applications and the Device Update agent. It uses Named Pipes (FIFOs) for IPC and must handle concurrent access safely.

## Race Condition Background

The API service tests previously suffered from race conditions when multiple test instances ran simultaneously. These issues manifested as:

- **FIFO Path Conflicts**: Multiple tests using the same hard-coded FIFO paths
- **Timing Dependencies**: Tests starting before the API service was ready
- **Resource Contention**: Concurrent access to shared temporary files

These race conditions were fixed through:
1. **Dynamic FIFO Paths**: Using process ID + timestamp for unique paths
2. **Synchronization**: Proper wait mechanisms for service readiness
3. **Robust Timeouts**: Multiple timeout layers to prevent hanging

## Basic Testing

### Prerequisites

Ensure the project is built with tests enabled:

```sh
# Build the entire project including tests
./scripts/build.sh -c -u

# Or build just the API service tests
cd out
cmake --build . --target apisvc_unit_tests
```

### Single Test Execution

```sh
# Navigate to build directory
cd out

# Run the API service unit tests
./bin/apisvc_unit_tests

# Run with verbose output for debugging
./bin/apisvc_unit_tests --reporter=verbose

# Run with specific test sections
./bin/apisvc_unit_tests --list-test-names-only
./bin/apisvc_unit_tests "apisvc crossproc tests"
```

### Expected Output

A successful test run should produce output similar to:

```
Randomness seeded to: 1234567890
WARNING: Unable to start file logger. (Log folder: /var/log/adu)
2025-11-02T19:48:28.5457Z 61782[61782] [D] svcstatus_set set new status to 3
2025-11-02T19:48:28.5459Z 61782[61782] [I] Initializing API Service thread with fifo: '/tmp/test_req_fifo_61782_75990945083387'
2025-11-02T19:48:28.9970Z 61782[61783] [I] Success opening API Request FIFO for read: '/tmp/test_req_fifo_61782_75990945083387'
...
===============================================================================
All tests passed (13 assertions in 1 test case)
```

## Race Condition Testing

### Basic Parallel Testing

Test the service with multiple concurrent instances to verify race condition fixes:

```sh
cd out

# Run 3 tests in parallel
for i in {1..3}; do ./bin/apisvc_unit_tests & done; wait

# Check exit status (should be 0 for all)
echo "All tests completed with status: $?"
```

### Stress Testing

#### Moderate Stress Test
```sh
cd out

# Run 5 tests in parallel with timeout protection
for i in {1..5}; do timeout 10s ./bin/apisvc_unit_tests & done; wait
```

#### Heavy Stress Test
```sh
cd out

# Run 10 tests in parallel with longer timeout
for i in {1..10}; do timeout 15s ./bin/apisvc_unit_tests & done; wait
```

#### Extended Stress Test
```sh
cd out

# Run multiple batches for extended testing
for batch in {1..5}; do
    echo "Running stress test batch $batch/5"
    for i in {1..6}; do timeout 20s ./bin/apisvc_unit_tests & done
    wait
    if [ $? -ne 0 ]; then
        echo "FAILURE: Race condition detected in batch $batch"
        exit 1
    fi
    echo "Batch $batch completed successfully"
done
echo "All stress test batches passed!"
```

#### Continuous Testing
```sh
cd out

# Continuous stress testing with failure detection
start_time=$(date +%s)
iteration=1

while true; do
    echo "Continuous test iteration $iteration ($(date))"

    # Run 3 parallel tests with timeout
    for i in {1..3}; do timeout 10s ./bin/apisvc_unit_tests & done
    wait

    if [ $? -ne 0 ]; then
        echo "FAILURE: Race condition detected in iteration $iteration"
        echo "Test duration: $(($(date +%s) - start_time)) seconds"
        exit 1
    fi

    # Stop after 30 iterations or 5 minutes
    if [ $iteration -ge 30 ] || [ $(($(date +%s) - start_time)) -ge 300 ]; then
        echo "Continuous testing completed successfully!"
        echo "Iterations: $iteration, Duration: $(($(date +%s) - start_time)) seconds"
        break
    fi

    iteration=$((iteration + 1))
    sleep 1
done
```

## Troubleshooting

### Common Issues

#### Test Timeouts
**Symptoms**: Tests are killed by timeout command
**Possible Causes**:
- Deadlock in FIFO communication
- Service not becoming ready
- Resource exhaustion

**Debug Steps**:
```sh
# Run without timeout to see where it hangs
./bin/apisvc_unit_tests

# Check for leaked processes
ps aux | grep apisvc_unit_tests

# Check for leaked FIFOs
ls -la /tmp/test_*fifo*
```

#### FIFO Conflicts
**Symptoms**: "Address already in use" or "File exists" errors
**Root Cause**: Multiple tests trying to use the same FIFO paths
**Solution**: The fix ensures unique paths per test instance

#### Service Not Ready
**Symptoms**: Tests fail during API service initialization
**Debug**: Add debug logging to see initialization timing

### Debugging Tools

#### System-Level Monitoring
```sh
# Monitor FIFO creation/deletion
inotifywait -m /tmp/ | grep fifo

# Monitor process creation
ps -ef | grep apisvc_unit_tests

# Check file descriptor usage
lsof | grep fifo
```

#### Test-Level Debugging
```sh
# Run single test with maximum verbosity
./bin/apisvc_unit_tests --reporter=verbose --success

# Run with specific logging level
ADUC_LOG_LEVEL=4 ./bin/apisvc_unit_tests
```

## CI/CD Integration

### GitHub Actions Example

```yaml
name: API Service Tests

on: [push, pull_request]

jobs:
  api-service-tests:
    runs-on: ubuntu-latest

    steps:
    - uses: actions/checkout@v3

    - name: Install Dependencies
      run: ./scripts/install-deps.sh -a

    - name: Build
      run: ./scripts/build.sh -c -u

    - name: Run Basic API Tests
      run: |
        cd out
        ./bin/apisvc_unit_tests

    - name: Run Race Condition Stress Test
      run: |
        cd out
        echo "Running parallel stress test..."
        for i in {1..5}; do timeout 30s ./bin/apisvc_unit_tests & done
        wait
        if [ $? -ne 0 ]; then
          echo "Race condition detected!"
          exit 1
        fi
        echo "Stress test passed!"
```

### Azure DevOps Pipeline Example

```yaml
- task: Bash@3
  displayName: 'API Service Race Condition Test'
  inputs:
    targetType: 'inline'
    script: |
      cd out

      # Basic functionality test
      ./bin/apisvc_unit_tests

      # Stress test for race conditions
      echo "Running stress test with 8 parallel instances..."
      for i in {1..8}; do timeout 30s ./bin/apisvc_unit_tests & done
      wait

      if [ $? -ne 0 ]; then
        echo "##vso[task.logissue type=error]Race condition detected in API service tests"
        exit 1
      fi

      echo "All API service tests passed including race condition verification"
```

## Performance Benchmarking

### Latency Testing

```sh
# Measure API call latency
cd out

# Single call timing
time ./bin/apisvc_unit_tests

# Multiple calls for average
for i in {1..10}; do
    time ./bin/apisvc_unit_tests 2>&1 | grep real
done
```

### Throughput Testing

```sh
# Measure concurrent throughput
cd out

start_time=$(date +%s.%3N)
for i in {1..20}; do ./bin/apisvc_unit_tests & done
wait
end_time=$(date +%s.%3N)

echo "20 concurrent tests completed in $(echo "$end_time - $start_time" | bc) seconds"
```

## Best Practices

1. **Always run stress tests** when modifying API service code
2. **Use timeouts** in automated testing to prevent hanging builds
3. **Monitor resource usage** during extended testing
4. **Clean up test artifacts** (/tmp/test_*fifo* files)
5. **Test on different platforms** (ARM, x86_64) if applicable
6. **Include race condition tests** in CI/CD pipelines
7. **Log test execution details** for debugging failures

## Additional Resources

- [API Service Implementation](../../src/agent/api/src/apisvc.c)
- [API Protocol Definition](../../src/utils/apiproto_utils/inc/aduc/apiproto.h)
- [SDK Documentation](./GetAduServiceStatus.md)
- [Main Testing Guide](../../README.md#run-tests)
