# SWUpdate Handler V3 Unit Test Updates

## Summary

Yes, the unit tests **needed significant updates** to support the swupdate-handler-v3 implementation. The v3/tests directory was copying v2 test files which used incompatible variable names and logic.

## What Changed

### 1. Test File Renamed
- **Old**: `swupdate_handler_v2_ut.cpp`
- **New**: `swupdate_handler_v3_ut.cpp`

### 2. Complete Test Rewrite
The v3 tests focus on **U-Boot variable management** and **boot health checking**, not script argument preparation like v2.

### 3. CMakeLists.txt Updated
```cmake
# Old:
project (swupdate_handler_unit_tests)
set (sources swupdate_handler_v2_ut.cpp ../src/swupdate_handler_v2.cpp ...)

# New:
project (swupdate_handler_v3_unit_tests)
set (sources swupdate_handler_v3_ut.cpp ../src/swupdate_handler_v3.cpp ...)
```

## Key Differences: V2 vs V3 Tests

### V2 Tests (Script-Based)
- Test workflow preparation
- Test script argument building
- Test script execution paths
- Focus: External script handling

### V3 Tests (U-Boot-Based)
- Test U-Boot variable management
- Test boot partition logic
- Test boot limit checks
- Test rollback scenarios
- Focus: Direct swupdate + U-Boot integration

## V3 Test Coverage

### 1. **Handler Creation** (`TEST_CASE: SWUpdate V3 Handler Creation`)
- Verifies handler factory creates valid handler instance

### 2. **Boot Partition Management** (`TEST_CASE: Boot Partition Management`)
- Tests detection of current partition (rootA/rootB)
- Tests target partition calculation
- Validates string-based partition names (not integers)

### 3. **Upgrade Available Flag** (`TEST_CASE: Upgrade Available Flag`)
- Tests setting upgrade_available to 1
- Tests clearing upgrade_available to 0

### 4. **Boot Attempts Tracking** (`TEST_CASE: Boot Attempts Tracking`)
- Tests boot_attempts initialization
- Tests boot_attempts increment
- **CRITICAL**: Tests boot limit check using `>=` operator (matches boot.cmd.in)
- Tests boot_attempts below, at, and above limit

### 5. **Boot Result Tracking** (`TEST_CASE: Boot Result Tracking`)
- Tests boot_result="success"
- Tests boot_result="failed"
- Tests boot_result="unknown"

### 6. **Partition-Specific Tracking** (3 test cases)
- Tests independent tracking of boot_result_A/B
- Tests independent tracking of boot_attempts_A/B
- Tests independent tracking of boot_timestamp_A/B

### 7. **Post-Reboot State Detection** (`TEST_CASE: Post-Reboot State Detection`)
- Tests fresh boot (no upgrade)
- Tests first boot after update
- Tests successful health check
- Tests failed health check with rollback

### 8. **Complete Workflow Simulation** (`TEST_CASE: Complete Update Workflow Simulation`)
- Tests full successful update flow
- Tests full failed update with automatic rollback

### 9. **Variable Name Compatibility** (`TEST_CASE: Variable Names Match boot.cmd.in`)
- **Critical verification test**
- Confirms v3 uses boot_attempts (not bootcount)
- Confirms v3 uses boot_result (not boot_successful)
- Confirms v3 uses string partitions (not integers)
- Confirms v3 doesn't use bootlimit variable (hardcoded to 3)
- Confirms v3 doesn't use fallback_partition variable

## Critical Alignment Verifications

### Boot Limit Check Logic
```cpp
// v3 matches boot.cmd.in exactly
constexpr int BOOT_LIMIT = 3;
if (bootAttempts >= BOOT_LIMIT)  // Triggers at exactly 3
{
    // Rollback
}

// boot.cmd.in:
# if test ${boot_attempts} -ge 3; then
```

### Variable Names
| Purpose | V2 Variable | V3 Variable | boot.cmd.in |
|---------|-------------|-------------|-------------|
| Attempt counter | bootcount | **boot_attempts** | boot_attempts |
| Success flag | boot_successful (0/1) | **boot_result** (string) | boot_result |
| Partition | 0/1 (integer) | **rootA/rootB** (string) | rootA/rootB |
| Boot limit | bootlimit (variable) | **BOOT_LIMIT=3** (const) | hardcoded 3 |

## Test Execution

The tests use mock U-Boot environment variables for isolated testing:

```cpp
// Mock storage
static std::map<std::string, std::string> mock_uboot_env;

// Test helpers
void SetMockUBootVar(const std::string& name, const std::string& value);
std::string GetMockUBootVar(const std::string& name);
void ClearMockUBootEnv();
```

## What's NOT Tested (Requires Hardware/Integration)

- Actual U-Boot environment variable reading/writing (fw_printenv/fw_setenv)
- Real swupdate binary execution
- Actual health check script execution
- Physical partition management
- Real system reboots
- Hardware-specific boot flow

## Next Steps for Full Testing

1. **Build Tests**: Compile with CMake to verify no syntax errors
2. **Run Unit Tests**: Execute to verify logic correctness
3. **Mock U-Boot Tools**: Add mocks for fw_printenv/fw_setenv if needed
4. **Integration Tests**: Test on actual hardware with real U-Boot
5. **Hardware Validation**: Verify full update cycle including reboots

## Files Modified

1. `/src/extensions/step_handlers/swupdate_handler_v3/tests/swupdate_handler_v3_ut.cpp` - NEW (replaced v2 version)
2. `/src/extensions/step_handlers/swupdate_handler_v3/tests/CMakeLists.txt` - UPDATED (references v3 files)

## Conclusion

The unit tests have been **completely updated** to:
- ✅ Test v3-specific U-Boot variable handling
- ✅ Verify alignment with boot.cmd.in
- ✅ Test boot limit logic (>= 3)
- ✅ Test string-based partition names
- ✅ Test partition-specific tracking
- ✅ Test complete update workflows
- ✅ Verify variable naming compatibility

The tests are now ready for compilation and execution to validate the v3 handler implementation.
