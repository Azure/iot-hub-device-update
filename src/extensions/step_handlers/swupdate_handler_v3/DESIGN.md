# SWUpdate Handler V3 - Architecture & Design Specification

## Overview

SWUpdate Handler V3 is a next-generation update handler for Azure Device Update that:
- Uses standard U-Boot environment variables (not rpipart-specific)
- Implements robust boot health checking and automatic rollback
- Generates its own extended error codes
- Depends only on the ADU Agent SDK (not the full agent)
- Supports A/B partition updates with fail-safe mechanisms

## Key Improvements Over V2

| Feature | V2 | V3 |
|---------|----|----|
| Bootloader Integration | Raspberry Pi specific (rpipart) | Generic U-Boot variables |
| Boot Health Check | None | Full health check with rollback |
| Error Codes | Uses agent error codes | Handler-specific error codes |
| Dependencies | Full agent utils | ADU Agent SDK only |
| Boot Safety | Manual intervention on failure | Automatic rollback |
| Partition Management | Script-based | Integrated U-Boot management |

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────┐
│             Azure Device Update Service                  │
└────────────────────┬────────────────────────────────────┘
                     │ Update Manifest
                     ▼
┌─────────────────────────────────────────────────────────┐
│               ADU Agent (Host Process)                   │
│  ┌──────────────────────────────────────────────┐      │
│  │         SWUpdate Handler V3 (.so)             │      │
│  │  ┌────────────────────────────────────────┐  │      │
│  │  │   Handler Core                          │  │      │
│  │  │   - Download Management                 │  │      │
│  │  │   - State Machine                       │  │      │
│  │  │   - Error Handling                      │  │      │
│  │  └──────────────┬─────────────────────────┘  │      │
│  │                 │                             │      │
│  │  ┌──────────────▼──────────┐  ┌────────────┐ │      │
│  │  │   U-Boot Manager         │  │Boot Health │ │      │
│  │  │   - GetEnv/SetEnv        │  │  Checker   │ │      │
│  │  │   - Partition Switch     │  │  - Health  │ │      │
│  │  │   - Boot Counter         │  │    Script  │ │      │
│  │  └──────────┬───────────────┘  └─────┬──────┘ │      │
│  └─────────────┼────────────────────────┼────────┘      │
└────────────────┼────────────────────────┼───────────────┘
                 │                        │
    ┌────────────▼────────┐  ┌───────────▼──────────┐
    │   ADU Agent SDK     │  │  Health Check Script │
    │   - Workflow API    │  │  - Service checks    │
    │   - Logging API     │  │  - Network checks    │
    │   - Process API     │  │  - File validation   │
    │   - System API      │  └──────────────────────┘
    └────────┬────────────┘
             │
    ┌────────▼────────────────────────────────────┐
    │           System Layer                      │
    │  ┌──────────┐  ┌──────────┐  ┌───────────┐ │
    │  │ U-Boot   │  │ SWUpdate │  │ Partitions│ │
    │  │ Env Vars │  │  Binary  │  │  (A/B)    │ │
    │  └──────────┘  └──────────┘  └───────────┘ │
    └─────────────────────────────────────────────┘
```

## Component Details

### 1. Handler Core

**File:** `src/swupdate_handler_v3.cpp`

**Responsibilities:**
- Implement ContentHandler interface
- Manage update workflow (Download, Install, Apply, Cancel)
- Coordinate between U-Boot manager and boot health checker
- Handle error reporting with extended result codes

**State Machine:**

```
                    ┌──────────┐
                    │  IDLE    │
                    └────┬─────┘
                         │ Download()
                    ┌────▼─────┐
                    │DOWNLOAD  │
                    │  ING     │
                    └────┬─────┘
                         │ Success
                    ┌────▼─────┐
                    │INSTALL   │
                    │  ING     │
                    └────┬─────┘
                         │ Success
                    ┌────▼─────┐
                    │APPLYING  │◄──── Reboot happens here
                    └────┬─────┘
                         │
                    ┌────▼─────┐
                    │HEALTH    │
                    │  CHECK   │
                    └────┬─────┘
                         │
                    ┌────▼─────┐
                    │SUCCESS/  │
                    │ROLLBACK  │
                    └──────────┘
```

**Key Methods:**

```cpp
class SWUpdateHandlerImpl : public ContentHandler
{
public:
    // ContentHandler interface
    ADUC_Result Download(ADUC_WorkflowHandle handle) override;
    ADUC_Result Install(ADUC_WorkflowHandle handle) override;
    ADUC_Result Apply(ADUC_WorkflowHandle handle) override;
    ADUC_Result Cancel(ADUC_WorkflowHandle handle) override;
    bool IsInstalled(ADUC_WorkflowHandle handle) override;

private:
    // Internal methods
    ADUC_Result CheckPostRebootState();
    ADUC_Result ExecuteSWUpdate(const string& swuFile);
    ADUC_Result PrepareForReboot();
    ADUC_Result HandleHealthCheckResult(bool passed);
};
```

### 2. U-Boot Manager

**File:** `src/uboot_manager.cpp`

**Purpose:** Abstraction layer for U-Boot environment variable management

**U-Boot Variables Used:**

| Variable | Type | Purpose |
|----------|------|---------|
| `boot_partition` | string (rootA/rootB) | Currently active boot partition |
| `upgrade_available` | int (0/1) | Flag indicating pending upgrade |
| `boot_attempts` | int | Number of boot attempts since upgrade |
| `boot_result` | string | Current boot result: "unknown", "success", "failed" |
| `boot_attempts_A` | int | Total boot attempts for rootA partition |
| `boot_attempts_B` | int | Total boot attempts for rootB partition |
| `boot_result_A` | string | Last result for rootA: "unknown", "success", "failed" |
| `boot_result_B` | string | Last result for rootB: "unknown", "success", "failed" |
| `boot_timestamp_A` | string | Last boot timestamp for rootA |
| `boot_timestamp_B` | string | Last boot timestamp for rootB |

**Note:** Boot limit is hardcoded to 3 in boot.cmd.in

**Interface:**

```cpp
class UBootManager
{
public:
    // Environment variable access
    static string GetEnv(const string& name);
    static bool SetEnv(const string& name, const string& value);
    static bool SaveEnv();
    
    // High-level partition management
    static string GetBootPartition();  // Returns "rootA" or "rootB"
    static bool SetBootPartition(const string& partition);
    
    // Upgrade state management
    static bool IsUpgradeAvailable();
    static bool SetUpgradeAvailable(bool available);
    
    // Boot counter management
    static int GetBootAttempts();
    static bool ResetBootAttempts();
    static int GetBootAttemptsForPartition(const string& partition);
    
    // Boot result tracking
    static string GetBootResult();  // Returns "unknown", "success", or "failed"
    static bool SetBootResult(const string& result);
    static string GetBootResultForPartition(const string& partition);
    static bool SetBootResultForPartition(const string& partition, const string& result);
    
private:
    static bool ExecuteFwSetEnv(const string& name, const string& value);
    static string ExecuteFwGetEnv(const string& name);
};
```

**Implementation Notes:**
- Uses `fw_setenv` and `fw_printenv` from `u-boot-fw-utils`
- Validates environment variable values
- Provides robust error handling
- Caches frequently accessed values

### 3. Boot Health Checker

**File:** `src/boot_health_checker.cpp`

**Purpose:** Verify system health after update and manage rollback

**Interface:**

```cpp
class BootHealthChecker
{
public:
    // Health check execution
    static ADUC_Result RunHealthCheck(const string& configPath);
    
    // Boot state verification
    static bool NeedsHealthCheck();
    static bool ShouldRollback();
    
    // Rollback management
    static ADUC_Result InitiateRollback();
    static ADUC_Result CompleteRollback();
    
private:
    static ADUC_Result ExecuteHealthCheckScript(const string& scriptPath);
    static bool ValidateSystemServices();
    static bool ValidateNetworkConnectivity();
    static bool ValidateCriticalFiles();
    static void LogHealthCheckResult(bool passed, const string& details);
};
```

**Health Check Criteria:**

1. **System Services**
   - ADU agent is running
   - Critical system services are active
   - No failed units

2. **Network Connectivity**
   - Can reach Azure IoT Hub
   - DNS resolution works
   - Network interface is up

3. **File System**
   - Critical files exist and are valid
   - Sufficient disk space
   - No corruption detected

4. **Custom Checks**
   - User-defined validation script
   - Application-specific checks

### 4. Boot Flow Manager

**Purpose:** Coordinate the complete update and boot verification flow

**Update Installation Flow:**

```
1. Download Phase:
   ├─ Download .swu files
   ├─ Validate hashes
   └─ Stage files in work folder

2. Install Phase:
   ├─ Determine target partition (inactive partition)
   ├─ Execute swupdate to write to target partition
   ├─ Verify swupdate success
   └─ Prepare U-Boot environment:
       ├─ Set boot_partition = target_partition ("rootA" or "rootB")
       ├─ Set upgrade_available = 1
       ├─ Set boot_attempts = 0
       ├─ Set boot_result = "unknown"
       └─ Save environment

3. Apply Phase:
   ├─ Verify environment is set correctly
   ├─ Log pre-reboot state
   └─ Trigger system reboot
```

**Post-Reboot Flow:**

```
1. U-Boot (runs before OS):
   ├─ Read boot_partition variable
   ├─ Boot from specified partition
   └─ Increment bootcount (if upgrade_available=1)

2. OS Boot + ADU Agent Start:
   └─ SWUpdate Handler V3 loaded

3. Handler Initialization:
   ├─ Check upgrade_available flag
   └─ If upgrade_available = 1:
       └─ Enter post-reboot verification mode

4. Post-Reboot Verification:
   ├─ Check bootcount
   ├─ If bootcount > bootlimit:
   │   ├─ Log "Boot limit exceeded"
   │   ├─ Initiate rollback
   │   └─ Reboot to fallback_partition
   └─ Else:
       ├─ Run health check
       └─ If health check passes:
           ├─ Set boot_successful = 1
           ├─ Set upgrade_available = 0
           ├─ Reset bootcount = 0
           ├─ Save environment
           └─ Report success to ADU service
       └─ If health check fails:
           ├─ Log failure details
           ├─ Bootcount will increment on next boot
           └─ Reboot (auto-retry or manual intervention)
```

## Configuration

### Handler Configuration File

**Location:** `/etc/adu/swupdate-handler-v3-config.json`

```json
{
    "description": "SWUpdate Handler V3 Configuration",
    "version": "3.0",
    
    "uboot": {
        "boot_partition_var": "boot_partition",
        "upgrade_available_var": "upgrade_available",
        "boot_attempts_var": "boot_attempts",
        "boot_result_var": "boot_result",
        "boot_attempts_a_var": "boot_attempts_A",
        "boot_attempts_b_var": "boot_attempts_B",
        "boot_result_a_var": "boot_result_A",
        "boot_result_b_var": "boot_result_B",
        "boot_limit": 3,
        "fw_env_config": "/etc/fw_env.config"
    },
    
    "health_check": {
        "enabled": true,
        "script_path": "/usr/lib/adu/scripts/boot-health-check.sh",
        "timeout_seconds": 120,
        "retry_on_failure": true,
        "max_retries": 2,
        "retry_delay_seconds": 10
    },
    
    "swupdate": {
        "binary_path": "/usr/bin/swupdate",
        "default_args": ["-v", "-e", "stable,copy1"],
        "timeout_seconds": 600,
        "log_file": "/var/log/swupdate.log"
    },
    
    "partitions": {
        "partition_a": "/dev/mmcblk0p2",
        "partition_b": "/dev/mmcblk0p3",
        "verify_after_write": true
    },
    
    "logging": {
        "log_level": "INFO",
        "log_to_file": true,
        "log_file_path": "/var/log/adu/swupdate-handler-v3.log",
        "max_log_size_mb": 10
    }
}
```

## Error Code Definitions

**File:** `swupdate_handler_v3_errors.json`

```json
{
    "handler_name": "swupdate_handler_v3",
    "facility_code": 10,
    "component_code": 3,
    "errors": [
        {
            "name": "SWUPDATE_V3_ERC_NONE",
            "value": 0,
            "description": "No error"
        },
        {
            "name": "SWUPDATE_V3_ERC_CONFIG_LOAD_FAILED",
            "value": 1,
            "description": "Failed to load handler configuration"
        },
        {
            "name": "SWUPDATE_V3_ERC_SCRIPT_NOT_FOUND",
            "value": 2,
            "description": "SWUpdate script file not found"
        },
        {
            "name": "SWUPDATE_V3_ERC_SWU_FILE_NOT_FOUND",
            "value": 3,
            "description": "SWU update file not found in payload"
        },
        {
            "name": "SWUPDATE_V3_ERC_UBOOT_ENV_READ_FAILED",
            "value": 10,
            "description": "Failed to read U-Boot environment variable"
        },
        {
            "name": "SWUPDATE_V3_ERC_UBOOT_ENV_WRITE_FAILED",
            "value": 11,
            "description": "Failed to write U-Boot environment variable"
        },
        {
            "name": "SWUPDATE_V3_ERC_UBOOT_ENV_SAVE_FAILED",
            "value": 12,
            "description": "Failed to save U-Boot environment"
        },
        {
            "name": "SWUPDATE_V3_ERC_INVALID_PARTITION",
            "value": 13,
            "description": "Invalid partition specified"
        },
        {
            "name": "SWUPDATE_V3_ERC_PARTITION_SWITCH_FAILED",
            "value": 14,
            "description": "Failed to switch boot partition"
        },
        {
            "name": "SWUPDATE_V3_ERC_BOOT_HEALTH_CHECK_FAILED",
            "value": 20,
            "description": "Boot health check failed"
        },
        {
            "name": "SWUPDATE_V3_ERC_HEALTH_CHECK_TIMEOUT",
            "value": 21,
            "description": "Health check script execution timeout"
        },
        {
            "name": "SWUPDATE_V3_ERC_HEALTH_CHECK_SCRIPT_NOT_FOUND",
            "value": 22,
            "description": "Health check script not found"
        },
        {
            "name": "SWUPDATE_V3_ERC_HEALTH_CHECK_SCRIPT_NOT_EXECUTABLE",
            "value": 23,
            "description": "Health check script is not executable"
        },
        {
            "name": "SWUPDATE_V3_ERC_BOOT_LIMIT_EXCEEDED",
            "value": 30,
            "description": "Boot attempt limit exceeded, initiating rollback"
        },
        {
            "name": "SWUPDATE_V3_ERC_ROLLBACK_FAILED",
            "value": 31,
            "description": "Automatic rollback failed"
        },
        {
            "name": "SWUPDATE_V3_ERC_ROLLBACK_NO_FALLBACK",
            "value": 32,
            "description": "No fallback partition available for rollback"
        },
        {
            "name": "SWUPDATE_V3_ERC_SWUPDATE_NOT_FOUND",
            "value": 100,
            "description": "SWUpdate binary not found"
        },
        {
            "name": "SWUPDATE_V3_ERC_SWUPDATE_EXEC_FAILED",
            "value": 101,
            "description": "SWUpdate execution failed"
        },
        {
            "name": "SWUPDATE_V3_ERC_SWUPDATE_TIMEOUT",
            "value": 102,
            "description": "SWUpdate execution timeout"
        },
        {
            "name": "SWUPDATE_V3_ERC_SWUPDATE_INVALID_EXIT_CODE",
            "value": 103,
            "description": "SWUpdate returned invalid exit code"
        },
        {
            "name": "SWUPDATE_V3_ERC_PARTITION_VERIFY_FAILED",
            "value": 110,
            "description": "Partition verification after write failed"
        },
        {
            "name": "SWUPDATE_V3_ERC_DOWNLOAD_FAILED",
            "value": 200,
            "description": "Download of update files failed"
        },
        {
            "name": "SWUPDATE_V3_ERC_FILE_VALIDATION_FAILED",
            "value": 201,
            "description": "Update file validation (hash check) failed"
        }
    ]
}
```

## Scripts

### 1. SWUpdate Wrapper Script

**File:** `scripts/swupdate-wrapper.sh`

```bash
#!/bin/bash
#
# SWUpdate Wrapper Script for ADU Handler V3
# Executes swupdate with proper arguments and error handling

set -e

SWUPDATE_BIN="${SWUPDATE_BIN:-/usr/bin/swupdate}"
SWU_FILE="$1"
LOG_FILE="${LOG_FILE:-/var/log/swupdate.log}"

# Validate inputs
if [ -z "$SWU_FILE" ]; then
    echo "Error: SWU file not specified" >&2
    exit 1
fi

if [ ! -f "$SWU_FILE" ]; then
    echo "Error: SWU file not found: $SWU_FILE" >&2
    exit 2
fi

if [ ! -x "$SWUPDATE_BIN" ]; then
    echo "Error: SWUpdate binary not found or not executable: $SWUPDATE_BIN" >&2
    exit 3
fi

# Execute swupdate
echo "Starting SWUpdate..."
echo "  Binary: $SWUPDATE_BIN"
echo "  SWU File: $SWU_FILE"
echo "  Log: $LOG_FILE"

"$SWUPDATE_BIN" -v -i "$SWU_FILE" 2>&1 | tee -a "$LOG_FILE"
EXIT_CODE=${PIPESTATUS[0]}

if [ $EXIT_CODE -eq 0 ]; then
    echo "SWUpdate completed successfully"
else
    echo "SWUpdate failed with exit code: $EXIT_CODE" >&2
fi

exit $EXIT_CODE
```

### 2. Boot Health Check Script

**File:** `scripts/boot-health-check.sh`

```bash
#!/bin/bash
#
# Boot Health Check Script for SWUpdate Handler V3
# Verifies system health after update

set -e

SCRIPT_NAME="boot-health-check"
EXIT_SUCCESS=0
EXIT_FAILURE=1

log() {
    logger -t "$SCRIPT_NAME" "$@"
    echo "$@"
}

check_critical_services() {
    log "Checking critical services..."
    
    local services=("deviceupdate-agent" "networking")
    
    for service in "${services[@]}"; do
        if ! systemctl is-active --quiet "$service"; then
            log "ERROR: Service $service is not running"
            return 1
        fi
        log "  ✓ $service is running"
    done
    
    return 0
}

check_network_connectivity() {
    log "Checking network connectivity..."
    
    # Check if network interface is up
    if ! ip link show | grep -q "state UP"; then
        log "ERROR: No network interface is up"
        return 1
    fi
    log "  ✓ Network interface is up"
    
    # Check DNS resolution
    if ! nslookup azure.com >/dev/null 2>&1; then
        log "WARNING: DNS resolution failed (non-critical)"
    else
        log "  ✓ DNS resolution works"
    fi
    
    return 0
}

check_filesystem() {
    log "Checking filesystem..."
    
    # Check root filesystem
    local root_usage=$(df / | tail -1 | awk '{print $5}' | sed 's/%//')
    if [ "$root_usage" -gt 95 ]; then
        log "ERROR: Root filesystem is ${root_usage}% full"
        return 1
    fi
    log "  ✓ Root filesystem has sufficient space (${root_usage}% used)"
    
    # Check critical files
    local critical_files=(
        "/usr/bin/AducIotAgent"
        "/etc/adu/du-config.json"
    )
    
    for file in "${critical_files[@]}"; do
        if [ ! -f "$file" ]; then
            log "ERROR: Critical file missing: $file"
            return 1
        fi
    done
    log "  ✓ All critical files present"
    
    return 0
}

check_custom_validation() {
    log "Running custom validation..."
    
    # Add custom application-specific checks here
    # Return 0 for success, 1 for failure
    
    return 0
}

# Main health check execution
main() {
    log "=== Starting Boot Health Check ==="
    log "Timestamp: $(date)"
    
    local overall_status=0
    
    if ! check_critical_services; then
        overall_status=1
    fi
    
    if ! check_network_connectivity; then
        overall_status=1
    fi
    
    if ! check_filesystem; then
        overall_status=1
    fi
    
    if ! check_custom_validation; then
        overall_status=1
    fi
    
    if [ $overall_status -eq 0 ]; then
        log "=== Boot Health Check PASSED ==="
    else
        log "=== Boot Health Check FAILED ==="
    fi
    
    return $overall_status
}

main
exit $?
```

## Dependencies

### Build Dependencies
- **ADU Agent SDK** (`aduagent-sdk`) - Provides all handler APIs
- **CMake** (>= 3.5)
- **C++ Compiler** (C++14 or later)

### Runtime Dependencies
- **swupdate** - Update execution binary
- **u-boot-fw-utils** - U-Boot environment tools (fw_setenv, fw_printenv)
- **deviceupdate-agent** - ADU agent runtime
- **libaduagent-sdk.so** - SDK runtime library

### Optional Dependencies
- **systemd** - For service management checks
- **iproute2** - For network checks

## Testing Strategy

### Unit Tests
- U-Boot environment variable management
- Boot health check logic
- Error code generation
- State machine transitions

### Integration Tests
- Full update flow on test system
- Health check pass/fail scenarios
- Rollback functionality
- Multiple sequential updates

### Hardware Tests
- Real device with U-Boot
- Actual partition switching
- Real reboot cycles
- Network failure scenarios

## Security Considerations

1. **Script Execution**
   - Health check script must be in trusted location
   - Verify script permissions before execution
   - Run scripts with minimal privileges

2. **U-Boot Environment**
   - Protect fw_env.config file
   - Validate environment variable values
   - Prevent unauthorized modifications

3. **Error Reporting**
   - Avoid leaking sensitive information in error messages
   - Log detailed errors locally, summary to cloud

## Performance Considerations

1. **Boot Time**
   - Health check should complete within 2 minutes
   - Minimize boot delay impact
   - Parallel health checks where possible

2. **Update Time**
   - SWUpdate execution time depends on image size
   - Typical: 5-15 minutes for full system update
   - Progress reporting to ADU service

3. **Resource Usage**
   - Minimal memory footprint
   - Efficient logging
   - Cleanup temporary files

## Backward Compatibility

### Migration from V2

**Configuration Changes:**
- New config file location and format
- U-Boot variable names different
- Health check configuration added

**Behavior Changes:**
- Automatic health checking enabled by default
- Automatic rollback on failure
- Different error codes

**Migration Path:**
1. Install V3 handler
2. Update config file
3. Configure U-Boot environment
4. Test update flow
5. Decommission V2

## Future Enhancements

1. **Advanced Health Checks**
   - Application-level health checks
   - Performance metrics validation
   - Custom validation plugins

2. **Rollback Improvements**
   - Partial rollback (data preservation)
   - Multi-generation rollback
   - Configurable rollback policies

3. **Monitoring**
   - Telemetry for health check results
   - Boot time tracking
   - Update success rate metrics

4. **Recovery**
   - USB recovery mode
   - Network recovery
   - Factory reset capability

---

**Document Version:** 1.0  
**Date:** January 4, 2026  
**Status:** Design Specification - Ready for Implementation
