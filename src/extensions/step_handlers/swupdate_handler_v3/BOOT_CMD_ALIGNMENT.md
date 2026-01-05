mv # SWUpdate Handler V3 - boot.cmd.in Alignment

## Summary

The swupdate-handler-v3 implementation has been updated to match the actual boot.cmd.in U-Boot script behavior.

## Variable Mapping

### Updated Variables (Now Aligned)

| Purpose | boot.cmd.in | Handler V3 | Status |
|---------|-------------|------------|--------|
| Boot partition identifier | `boot_partition` (string: "rootA"/"rootB") | `boot_partition` (string: "rootA"/"rootB") | ✅ **ALIGNED** |
| Boot attempt counter | `boot_attempts` | `boot_attempts` | ✅ **ALIGNED** |
| Boot limit | Hardcoded `3` | `BOOT_LIMIT` constant = 3 | ✅ **ALIGNED** |
| Upgrade flag | `upgrade_available` ("0"/"1") | `upgrade_available` ("0"/"1") | ✅ **ALIGNED** |
| Boot result | `boot_result` ("unknown"/"success"/"failed") | `boot_result` ("unknown"/"success"/"failed") | ✅ **ALIGNED** |
| Partition A attempts | `boot_attempts_A` | `boot_attempts_A` | ✅ **ALIGNED** |
| Partition B attempts | `boot_attempts_B` | `boot_attempts_B` | ✅ **ALIGNED** |
| Partition A result | `boot_result_A` | `boot_result_A` | ✅ **ALIGNED** |
| Partition B result | `boot_result_B` | `boot_result_B` | ✅ **ALIGNED** |
| Timestamps A | `boot_timestamp_A` | `boot_timestamp_A` (recognized) | ✅ **ALIGNED** |
| Timestamps B | `boot_timestamp_B` | `boot_timestamp_B` (recognized) | ✅ **ALIGNED** |

### Removed Variables (Not in boot.cmd.in)

These were removed from the handler as they don't exist in boot.cmd.in:
- ❌ `bootcount` (replaced with `boot_attempts`)
- ❌ `bootlimit` (now hardcoded constant)
- ❌ `boot_successful` (replaced with `boot_result="success"`)
- ❌ `fallback_partition` (boot.cmd implicitly switches A↔B)

## Boot Flow Alignment

### 1. Update Installation (Handler → U-Boot)

**Handler Actions:**
```cpp
// Install phase sets up for reboot
SetUBootEnv("boot_partition", targetPartition);  // "rootA" or "rootB"
SetUBootEnv("upgrade_available", "1");
SetUBootEnv("boot_attempts", "0");
SetUBootEnv("boot_result", "unknown");
```

**boot.cmd.in Expects:**
- ✅ `boot_partition` = "rootA" or "rootB" (string)
- ✅ `upgrade_available` = "1"
- ✅ `boot_attempts` initialized

### 2. U-Boot Boot Process (boot.cmd.in)

**boot.cmd.in Logic:**
```bash
if test "${upgrade_available}" = "1"; then
    if test "${boot_attempts}" -ge "3"; then
        # Mark current partition as failed
        setenv boot_result failed
        # Rollback: switch partition
        if test "${boot_partition}" = "rootA"; then
            setenv boot_partition rootB
        else
            setenv boot_partition rootA
        fi
        setenv boot_attempts 0
        setenv upgrade_available 0
    fi
fi

# Increment boot attempts
setexpr boot_attempts ${boot_attempts} + 1
```

**Handler Understands:**
- ✅ Checks `boot_attempts >= 3` (same comparison)
- ✅ Knows boot.cmd will auto-rollback if limit exceeded
- ✅ Works with string partition names

### 3. Post-Reboot Health Check (Handler)

**Handler Actions:**
```cpp
// Check if we need health check
string upgradeAvailable = GetUBootEnv("upgrade_available");
if (upgradeAvailable == "1") {
    // Check boot attempts
    int bootAttempts = GetBootAttempts();
    if (bootAttempts >= BOOT_LIMIT) {
        // Boot limit reached - boot.cmd should have rolled back
        Log_Error("Boot limit exceeded");
        return failure;
    }
    
    // Run health check
    if (healthCheckPassed) {
        SetUBootEnv("boot_result", "success");
        SetUBootEnv("upgrade_available", "0");
        SetUBootEnv("boot_attempts", "0");
        
        // Update partition-specific result
        if (bootPartition == "rootA")
            SetUBootEnv("boot_result_A", "success");
        else
            SetUBootEnv("boot_result_B", "success");
    } else {
        SetUBootEnv("boot_result", "failed");
        // Will reboot, boot.cmd will increment boot_attempts
    }
}
```

**Alignment with boot.cmd.in:**
- ✅ Respects boot.cmd's rollback mechanism
- ✅ Sets appropriate result flags
- ✅ Uses correct variable names and values

## Update Workflow

### Complete Flow (Handler + boot.cmd.in)

```
┌─────────────────────────────────────────────────────────┐
│ 1. Handler Install Phase                                 │
│    - Execute swupdate                                    │
│    - Set boot_partition="rootB" (if currently rootA)     │
│    - Set upgrade_available="1"                           │
│    - Set boot_attempts="0"                               │
│    - Set boot_result="unknown"                           │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼ Reboot
┌─────────────────────────────────────────────────────────┐
│ 2. U-Boot (boot.cmd.in) - First Boot Attempt            │
│    - Checks upgrade_available="1" → validation mode     │
│    - Checks boot_attempts=0 < 3 → OK to try            │
│    - Increments boot_attempts → 1                       │
│    - Boots from rootB                                    │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼ OS Starts
┌─────────────────────────────────────────────────────────┐
│ 3. Handler Post-Reboot Check                            │
│    - Detects upgrade_available="1"                      │
│    - Checks boot_attempts=1 < 3 → OK                   │
│    - Runs health check                                   │
│    - If PASS:                                            │
│      • Set boot_result="success"                        │
│      • Set boot_result_B="success"                      │
│      • Set upgrade_available="0"                        │
│      • Set boot_attempts="0"                            │
│      → Update complete ✓                                │
│    - If FAIL:                                            │
│      • Set boot_result="failed"                         │
│      • Reboot → boot.cmd will retry                     │
└─────────────────────────────────────────────────────────┘

If health check fails 3 times:
┌─────────────────────────────────────────────────────────┐
│ U-Boot (boot.cmd.in) - Fourth Boot Attempt              │
│    - Checks upgrade_available="1" → validation mode     │
│    - Checks boot_attempts=3 >= 3 → ROLLBACK!           │
│    - Sets boot_result="failed"                          │
│    - Switches boot_partition: rootB → rootA             │
│    - Resets boot_attempts="0"                           │
│    - Sets upgrade_available="0"                         │
│    - Boots from rootA (previous working partition)      │
│    → Automatic rollback complete                        │
└─────────────────────────────────────────────────────────┘
```

## Code Changes Made

### 1. swupdate_handler_v3.cpp

**Variable Definitions Updated:**
```cpp
// OLD (not matching boot.cmd.in):
#define UBOOT_VAR_BOOTCOUNT "bootcount"
#define UBOOT_VAR_BOOTLIMIT "bootlimit"
#define UBOOT_VAR_BOOT_SUCCESSFUL "boot_successful"
#define UBOOT_VAR_FALLBACK_PARTITION "fallback_partition"

// NEW (matching boot.cmd.in):
#define UBOOT_VAR_BOOT_PARTITION "boot_partition"      // "rootA" or "rootB"
#define UBOOT_VAR_BOOT_ATTEMPTS "boot_attempts"
#define UBOOT_VAR_BOOT_RESULT "boot_result"            // "unknown", "success", "failed"
#define UBOOT_VAR_BOOT_RESULT_A "boot_result_A"
#define UBOOT_VAR_BOOT_RESULT_B "boot_result_B"
#define UBOOT_VAR_BOOT_ATTEMPTS_A "boot_attempts_A"
#define UBOOT_VAR_BOOT_ATTEMPTS_B "boot_attempts_B"
#define BOOT_LIMIT 3  // Hardcoded like in boot.cmd.in
```

**Install() Updated:**
```cpp
// OLD:
SetUBootEnv("boot_partition", std::to_string(targetPartition)); // 0 or 1
SetUBootEnv("fallback_partition", std::to_string(currentPartition));
SetUBootEnv("bootcount", "0");
SetUBootEnv("boot_successful", "0");

// NEW:
SetUBootEnv("boot_partition", targetPartition);  // "rootA" or "rootB"
SetUBootEnv("upgrade_available", "1");
SetUBootEnv("boot_attempts", "0");
SetUBootEnv("boot_result", "unknown");
```

**CheckPostRebootState() Updated:**
```cpp
// OLD:
int bootCount = std::stoi(GetUBootEnv("bootcount"));
if (bootCount > bootLimit) { ... }
SetUBootEnv("boot_successful", "1");

// NEW:
int bootAttempts = std::stoi(GetUBootEnv("boot_attempts"));
if (bootAttempts >= BOOT_LIMIT) { ... }
SetUBootEnv("boot_result", "success");
if (bootPartition == "rootA")
    SetUBootEnv("boot_result_A", "success");
```

**IsInstalled() Updated:**
```cpp
// OLD:
string bootSuccessful = GetUBootEnv("boot_successful");
if (bootSuccessful == "1" && upgradeAvailable == "0")

// NEW:
string bootResult = GetUBootEnv("boot_result");
if (bootResult == "success" && upgradeAvailable == "0")
```

### 2. DESIGN.md

Updated documentation to reflect:
- Variable table with correct names and types
- Boot flow matching boot.cmd.in logic
- Configuration examples with correct variables
- Removed references to non-existent variables

### 3. README.md

Updated user documentation with:
- Correct variable names and values
- Updated boot flow description
- Fixed configuration examples
- Corrected troubleshooting commands

## Testing Checklist

To verify alignment:

- [ ] Build handler successfully
- [ ] Deploy to device with boot.cmd.in
- [ ] Verify U-Boot variables initialized on first boot
- [ ] Test successful update flow
- [ ] Test health check failure and retry
- [ ] Test automatic rollback after 3 failed attempts
- [ ] Verify partition-specific result tracking
- [ ] Check logs match expected behavior

## Compatibility

✅ **Handler V3 is now fully compatible with boot.cmd.in**

The handler will:
1. Set variables boot.cmd.in expects
2. Read variables boot.cmd.in sets
3. Understand boot.cmd.in's rollback behavior
4. Work correctly with string partition names ("rootA"/"rootB")
5. Respect the hardcoded boot limit of 3

---

**Date:** January 5, 2026  
**Status:** ✅ Aligned and Ready for Testing
