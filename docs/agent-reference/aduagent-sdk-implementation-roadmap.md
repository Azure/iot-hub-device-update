# ADU Agent SDK Implementation Roadmap

## Quick Start Summary

This document provides a detailed implementation plan for creating the ADU Agent SDK and refactoring the swupdate-handler-v3 to use it.

## Prerequisites

- Understanding of the ADU agent architecture
- CMake build system knowledge
- Yocto/BitBake recipe development experience
- C/C++ development skills

## Phase 1: SDK Foundation (Weeks 1-2)

### Task 1.1: Create SDK Directory Structure

**Location:** `/home/nox/adu_yocto/sources/iot-hub-device-update/src/aduagent-sdk/`

```bash
mkdir -p src/aduagent-sdk/{inc/aduc,src,error_code_generator,examples,cmake}
```

**Files to create:**
- `CMakeLists.txt` - Main build configuration
- `README.md` - SDK usage documentation
- `inc/aduc/*.h` - Public header files
- `src/*.c` - Implementation files
- `error_code_generator/generate_handler_errors.py` - Error code generator
- `cmake/aduagent-sdk-config.cmake.in` - CMake package config

### Task 1.2: Implement SDK Headers

Create the following header files with clear, stable APIs:

1. **sdk_result.h** - Result types and error handling
2. **sdk_types.h** - Common types (WorkflowHandle, FileEntity, etc.)
3. **sdk_logging.h** - Logging interface
4. **sdk_workflow.h** - Workflow data access
5. **sdk_process.h** - Process execution
6. **sdk_string.h** - String utilities
7. **sdk_system.h** - System/file operations
8. **sdk_config.h** - Configuration access
9. **sdk_content_handler.h** - Handler interface contract

### Task 1.3: Implement SDK Source Files

Create wrapper implementations that call into existing agent utilities:

```
src/sdk_result.c       -> Wraps result.h functionality
src/sdk_logging.c      -> Wraps logging utilities
src/sdk_workflow.c     -> Wraps workflow_utils
src/sdk_process.c      -> Wraps process_utils
src/sdk_string.c       -> Wraps string_utils
src/sdk_system.c       -> Wraps system_utils
src/sdk_config.c       -> Wraps config_utils
```

**Key principle:** SDK functions are thin wrappers that provide a stable API while internally calling agent utilities.

### Task 1.4: Create Error Code Generator

**File:** `error_code_generator/generate_handler_errors.py`

This Python script:
- Takes a JSON file with error definitions
- Generates a C header file with error code macros
- Uses the `MAKE_ADUC_EXTENDEDRESULTCODE` macro
- Includes documentation comments

**Example usage:**
```bash
python3 generate_handler_errors.py swupdate_v3_errors.json swupdate_v3_errors.h
```

### Task 1.5: Write SDK CMakeLists.txt

Key requirements:
- Build both shared and static library versions
- Install headers to include/aduc/
- Install libraries to lib/
- Generate CMake package config files
- Support find_package(aduagent-sdk)

### Task 1.6: SDK Unit Tests

Create basic unit tests:
```
tests/
  ├── test_sdk_result.c
  ├── test_sdk_workflow.c
  ├── test_sdk_process.c
  └── test_sdk_system.c
```

### Deliverable 1: Checkpoint

**Verification:**
```bash
cd src/aduagent-sdk
mkdir build && cd build
cmake ..
make
make test
```

**Expected output:**
- `libaduagent-sdk.so` created
- `libaduagent-sdk.a` created
- All unit tests pass
- Headers available in inc/aduc/

## Phase 2: Refactor Agent for SDK (Week 3)

### Task 2.1: Update Top-Level CMakeLists.txt

Add SDK subdirectory to main project:

```cmake
# In /src/CMakeLists.txt
add_subdirectory(aduagent-sdk)
```

### Task 2.2: Create Agent-SDK Bridge

Some SDK functions need to access internal agent state. Create a bridge layer:

**File:** `src/aduagent-sdk/src/sdk_agent_bridge.h` (internal, not installed)

```c
// Internal bridge - not part of public SDK
// Allows SDK to access agent internals

// Called by agent during initialization
void sdk_bridge_init(void* agent_context);

// Called by agent during shutdown
void sdk_bridge_uninit(void);
```

### Task 2.3: Implement SDK Functions

For each SDK function, implement using existing agent utilities:

**Example:** `sdk_workflow_get_id()`

```c
const char* sdk_workflow_get_id(ADUC_WorkflowHandle handle)
{
    return workflow_peek_id((ADUC_WorkflowDataHandle)handle);
}
```

**Example:** `sdk_uboot_getenv()`

```c
char* sdk_uboot_getenv(const char* name)
{
    // Use fw_getenv or libubootenv
    // This is new functionality specific to v3 handler
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "fw_printenv -n %s", name);
    
    FILE* fp = popen(cmd, "r");
    if (!fp) return NULL;
    
    char value[1024];
    if (fgets(value, sizeof(value), fp) != NULL) {
        pclose(fp);
        // Remove trailing newline
        size_t len = strlen(value);
        if (len > 0 && value[len-1] == '\n')
            value[len-1] = '\0';
        return strdup(value);
    }
    
    pclose(fp);
    return NULL;
}
```

### Task 2.4: Test SDK Integration

Verify SDK can be used within agent build:

```bash
cd /src/extensions/step_handlers/swupdate_handler_v2
# Temporarily modify to use SDK functions
# Verify it still builds and works
```

### Deliverable 2: Checkpoint

**Verification:**
- Agent builds successfully with SDK
- SDK library is created
- Existing handlers still work
- SDK can access necessary agent functionality

## Phase 3: Yocto Packaging (Week 4)

### Task 3.1: Create deviceupdate-agent-dev Recipe

**Location:** `meta-azure-device-update/recipes-azure/deviceupdate-agent-dev/`

**Files:**
```
deviceupdate-agent-dev_1.0.0.bb
files/
  └── deviceupdate-agent-dev.pc.in
```

### Task 3.2: Recipe Implementation

The recipe should:
1. Build only the SDK (not the full agent)
2. Install SDK headers to `/usr/include/aduc/`
3. Install SDK libraries to `/usr/lib/`
4. Install error code generator to `/usr/share/aduagent-sdk/`
5. Install CMake config files
6. Install pkg-config file

**Key CMake flag:**
```cmake
EXTRA_OECMAKE = "-DADUC_BUILD_SDK_ONLY=ON"
```

### Task 3.3: Add BUILD_SDK_ONLY Option

Update main CMakeLists.txt:

```cmake
option(ADUC_BUILD_SDK_ONLY "Build only the SDK, not the agent" OFF)

if(ADUC_BUILD_SDK_ONLY)
    # Build only SDK
    add_subdirectory(src/aduagent-sdk)
    return()
endif()

# ... rest of agent build
```

### Task 3.4: Test SDK Package

Build and install the package:

```bash
bitbake deviceupdate-agent-dev
bitbake deviceupdate-agent-dev -c devshell

# In devshell:
ls /usr/include/aduc/
ls /usr/lib/libaduagent-sdk*
ls /usr/share/aduagent-sdk/
```

### Task 3.5: Create Example Handler Recipe

Create a minimal example handler that uses only the SDK:

**Location:** `meta-azure-device-update/recipes-azure/example-handler/`

This verifies:
- SDK headers are accessible
- SDK library can be linked
- CMake find_package() works
- Handler can build without agent sources

### Deliverable 3: Checkpoint

**Verification:**
- `deviceupdate-agent-dev` package builds successfully
- Package contains all necessary files
- Example handler can build using only SDK package
- No agent source code required for handler build

## Phase 4: SWUpdate Handler V3 Implementation (Weeks 5-6)

### Task 4.1: Create Handler Structure

```bash
cd /src/extensions/step_handlers/swupdate_handler_v3
mkdir -p inc src scripts tests
```

### Task 4.2: Define Error Codes

**File:** `swupdate_handler_v3_errors.json`

```json
{
    "handler_name": "swupdate_handler_v3",
    "facility_code": 10,
    "component_code": 3,
    "errors": [
        {"name": "SWUPDATE_V3_ERC_SCRIPT_NOT_FOUND", "value": 1},
        {"name": "SWUPDATE_V3_ERC_UBOOT_ENV_READ_FAILED", "value": 3},
        {"name": "SWUPDATE_V3_ERC_UBOOT_ENV_WRITE_FAILED", "value": 4},
        {"name": "SWUPDATE_V3_ERC_BOOT_HEALTH_CHECK_FAILED", "value": 5},
        {"name": "SWUPDATE_V3_ERC_PARTITION_SWITCH_FAILED", "value": 6},
        {"name": "SWUPDATE_V3_ERC_SWUPDATE_EXEC_FAILED", "value": 100}
    ]
}
```

### Task 4.3: Implement U-Boot Manager

**File:** `src/uboot_manager.cpp`

Functions:
- `UBootGetEnv(const char* name)`
- `UBootSetEnv(const char* name, const char* value)`
- `UBootSaveEnv()`
- `GetBootPartition()`
- `SetBootPartition(int partition)`
- `IsUpgradeAvailable()`
- `SetUpgradeAvailable(bool available)`

Uses `fw_setenv` and `fw_printenv` tools.

### Task 4.4: Implement Boot Health Checker

**File:** `src/boot_health_checker.cpp`

Functions:
- `RunHealthCheck()` - Execute health check script
- `IsBootSuccessful()` - Check if boot was successful
- `MarkBootSuccessful()` - Mark current boot as good
- `CheckBootCount()` - Verify boot attempts
- `InitiateRollback()` - Rollback to previous partition

### Task 4.5: Implement Main Handler

**File:** `src/swupdate_handler_v3.cpp`

Key methods:
- `Download()` - Download SWU files
- `Install()` - Execute swupdate, manage partitions
- `Apply()` - Trigger reboot
- `Cancel()` - Handle cancellation
- `IsInstalled()` - Check installed criteria

**Boot flow:**
1. Check if we're recovering from reboot (upgrade_available=1)
2. If yes, run health check
3. If health check passes, mark successful
4. If health check fails, check boot count and rollback if needed

### Task 4.6: Create Scripts

**File:** `scripts/swupdate-wrapper.sh`

Wrapper script that:
- Validates environment
- Executes swupdate with proper arguments
- Handles logging
- Returns appropriate exit codes

**File:** `scripts/boot-health-check.sh`

Health check script that:
- Checks system services
- Verifies network connectivity
- Validates critical files
- Returns 0 for success, non-zero for failure

### Task 4.7: Update Handler CMakeLists.txt

Key changes from v2:
```cmake
# Use SDK instead of agent utilities
find_package(aduagent-sdk REQUIRED)

target_link_libraries(${target_name}
    PRIVATE
        aduagent::aduagent-sdk  # Only SDK dependency
)

# Generate error codes
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/inc/swupdate_handler_v3_errors.h
    COMMAND python3 ${ADUAGENT_SDK_ERROR_GENERATOR}
            ${CMAKE_CURRENT_SOURCE_DIR}/swupdate_handler_v3_errors.json
            ${CMAKE_CURRENT_SOURCE_DIR}/inc/swupdate_handler_v3_errors.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/swupdate_handler_v3_errors.json
)
```

### Task 4.8: Create Handler Configuration

**File:** `swupdate-handler-v3-config.json`

Configuration for:
- U-Boot variable names
- Boot limits
- Health check settings
- SWUpdate binary location

### Task 4.9: Unit Tests

**Location:** `tests/unit_tests/`

Tests for:
- U-Boot environment management
- Boot health check logic
- Error code generation
- Handler state machine

### Deliverable 4: Checkpoint

**Verification:**
```bash
# Build handler independently
mkdir handler-build
cd handler-build
cmake ../src/extensions/step_handlers/swupdate_handler_v3 \
    -DCMAKE_PREFIX_PATH=/usr/lib/cmake/aduagent-sdk
make
```

**Expected:**
- Handler builds successfully
- Only depends on SDK library
- Error codes generated correctly
- All unit tests pass

## Phase 5: Testing & Documentation (Week 7)

### Task 5.1: Integration Tests

**Test scenarios:**
1. **Normal Update Flow**
   - Install update to partition B
   - Reboot
   - Health check passes
   - Update marked successful

2. **Failed Health Check with Recovery**
   - Install update
   - Reboot
   - Health check fails
   - Boot count increments
   - Rollback after boot limit

3. **Interrupted Update**
   - Start update
   - Cancel mid-installation
   - Verify system stability

4. **Multiple Sequential Updates**
   - Update A→B
   - Update B→A
   - Verify partition switching

### Task 5.2: Hardware Testing

Test on actual Raspberry Pi with:
- Real U-Boot environment
- Real partitions
- Real reboot cycles
- Network connectivity

### Task 5.3: Documentation

Create the following documents:

1. **Handler Development Guide**
   - How to use SDK
   - Creating custom handlers
   - Error code generation
   - Best practices

2. **API Reference**
   - Complete SDK API documentation
   - Function descriptions
   - Usage examples
   - Code samples

3. **Migration Guide**
   - Migrating from v2 to v3
   - U-Boot variable changes
   - Configuration differences

4. **SWUpdate Handler V3 User Guide**
   - Installation instructions
   - Configuration options
   - Troubleshooting
   - Update manifest examples

### Task 5.4: Create Example Project

Standalone handler project showing:
- How to use SDK
- CMake setup
- Error code generation
- Testing approach

**Location:** `examples/standalone-handler/`

### Deliverable 5: Final Checkpoint

**Acceptance Criteria:**
- [ ] Handler passes all integration tests
- [ ] Handler works on real hardware
- [ ] Complete documentation available
- [ ] Example project builds and runs
- [ ] No dependencies on agent internals
- [ ] Error codes properly generated
- [ ] BitBake recipe works

## Directory Structure Summary

Final structure after all phases:

```
iot-hub-device-update/
├── docs/
│   └── agent-reference/
│       ├── aduagent-sdk-design.md
│       ├── aduagent-sdk-api-reference.md
│       ├── handler-development-guide.md
│       └── swupdate-v3-migration-guide.md
├── src/
│   ├── aduagent-sdk/
│   │   ├── CMakeLists.txt
│   │   ├── README.md
│   │   ├── inc/aduc/
│   │   │   ├── sdk_result.h
│   │   │   ├── sdk_types.h
│   │   │   ├── sdk_logging.h
│   │   │   ├── sdk_workflow.h
│   │   │   ├── sdk_process.h
│   │   │   ├── sdk_string.h
│   │   │   ├── sdk_system.h
│   │   │   ├── sdk_config.h
│   │   │   └── sdk_content_handler.h
│   │   ├── src/
│   │   │   ├── sdk_result.c
│   │   │   ├── sdk_logging.c
│   │   │   ├── sdk_workflow.c
│   │   │   ├── sdk_process.c
│   │   │   ├── sdk_string.c
│   │   │   ├── sdk_system.c
│   │   │   └── sdk_config.c
│   │   ├── error_code_generator/
│   │   │   ├── generate_handler_errors.py
│   │   │   └── error_code_template.h
│   │   ├── examples/
│   │   │   └── minimal_handler/
│   │   └── tests/
│   └── extensions/step_handlers/
│       └── swupdate_handler_v3/
│           ├── CMakeLists.txt
│           ├── README.md
│           ├── swupdate_handler_v3_errors.json
│           ├── swupdate-handler-v3-config.json
│           ├── inc/
│           │   ├── swupdate_handler_v3.hpp
│           │   └── swupdate_handler_v3_errors.h (generated)
│           ├── src/
│           │   ├── handler_create.cpp
│           │   ├── swupdate_handler_v3.cpp
│           │   ├── uboot_manager.cpp
│           │   └── boot_health_checker.cpp
│           ├── scripts/
│           │   ├── swupdate-wrapper.sh
│           │   └── boot-health-check.sh
│           └── tests/
└── meta-azure-device-update/
    └── recipes-azure/
        ├── deviceupdate-agent-dev/
        │   └── deviceupdate-agent-dev_1.0.0.bb
        └── swupdate-handler-v3/
            └── swupdate-handler-v3_1.0.0.bb
```

## Build Flow

### Agent Build (with SDK)
```bash
cd iot-hub-device-update
mkdir build && cd build
cmake ..
make
# Produces: libaduagent-sdk.so + agent + handlers
```

### SDK-Only Build (for packaging)
```bash
cmake .. -DADUC_BUILD_SDK_ONLY=ON
make
# Produces: only libaduagent-sdk.so
```

### Handler Build (standalone)
```bash
# After SDK is installed
cd swupdate_handler_v3
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/usr
make
# Produces: microsoft_swupdate_3.so
```

## Key Design Principles

1. **Stability**: SDK API must remain stable across versions
2. **Simplicity**: SDK should be simple and easy to use
3. **Independence**: Handlers should not depend on agent internals
4. **Extensibility**: Easy to add new SDK functions
5. **Documentation**: Complete documentation for all SDK APIs
6. **Testing**: Comprehensive tests at all levels
7. **Packaging**: Clean separation between dev and runtime packages

## Success Criteria

- [ ] SDK package can be installed independently
- [ ] Handler can be built with only SDK installed (no agent sources)
- [ ] Handler generates own error codes
- [ ] Handler uses U-Boot variables instead of rpipart
- [ ] Handler implements boot health checking
- [ ] All tests pass
- [ ] Documentation is complete
- [ ] Example project demonstrates usage

## Next Steps

1. Review this roadmap with team
2. Get approval for approach
3. Create GitHub issues for each phase
4. Assign owners for each phase
5. Begin Phase 1 implementation

---

**Document Version:** 1.0  
**Date:** January 4, 2026  
**Status:** Ready for Implementation
