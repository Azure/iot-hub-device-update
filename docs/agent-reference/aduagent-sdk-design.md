# ADU Agent SDK Design Document

## Overview

This document outlines the design for the ADU Agent SDK - a development package that enables device builders to create custom update handlers without building the entire ADU agent project. The primary goal is to decouple handler development from the agent's core implementation while maintaining clean interfaces and dependencies.

## Problem Statement

Currently, developing custom handlers (like swupdate-handler-v3) requires:
1. Building the entire ADU agent project
2. Deep coupling with internal agent utilities
3. No clear separation between public SDK APIs and internal implementation details
4. Difficulty in distributing handler development capabilities to device builders

## Solution: ADU Agent SDK

### Architecture Goals

1. **Minimal Dependencies**: Handlers should depend only on stable SDK interfaces
2. **Self-Contained Error Handling**: Each handler generates its own extended error codes
3. **Independent Builds**: Handlers can be built separately with only the SDK package installed
4. **Clear Interfaces**: Well-defined public APIs for handler development
5. **Backward Compatibility**: Maintain compatibility with existing handlers during transition

## SDK Structure

### Directory Layout

```
/src/aduagent-sdk/
├── CMakeLists.txt                    # SDK build configuration
├── README.md                         # SDK usage documentation
├── inc/                              # Public SDK headers
│   └── aduc/
│       ├── sdk_result.h             # Result types and error code macros
│       ├── sdk_types.h              # Common types (WorkflowHandle, FileEntity, etc.)
│       ├── sdk_logging.h            # Logging interface
│       ├── sdk_workflow.h           # Workflow data access APIs
│       ├── sdk_process.h            # Process execution utilities
│       ├── sdk_string.h             # String manipulation utilities
│       ├── sdk_system.h             # System utilities (file, env vars)
│       ├── sdk_config.h             # Configuration access
│       └── sdk_content_handler.h    # Content handler interface/contract
├── src/                              # SDK implementation
│   ├── sdk_result.c                 # Result handling implementation
│   ├── sdk_logging.c                # Logging wrapper implementation
│   ├── sdk_workflow.c               # Workflow access wrappers
│   ├── sdk_process.c                # Process utilities
│   ├── sdk_string.c                 # String utilities
│   ├── sdk_system.c                 # System utilities
│   └── sdk_config.c                 # Config utilities
└── error_code_generator/            # Error code generation tools
    ├── generate_handler_errors.py   # Script to generate handler-specific error codes
    └── error_code_template.h        # Template for handler error codes
```

### Package Structure: deviceupdate-agent-dev

The SDK will be packaged as `deviceupdate-agent-dev` with the following structure:

```
/usr/include/aduc/                   # Public SDK headers
    sdk_result.h
    sdk_types.h
    sdk_logging.h
    sdk_workflow.h
    sdk_process.h
    sdk_string.h
    sdk_system.h
    sdk_config.h
    sdk_content_handler.h

/usr/lib/                            # SDK library
    libaduagent-sdk.so
    libaduagent-sdk.a                # Static version for handlers

/usr/lib/cmake/aduagent-sdk/         # CMake config files
    aduagent-sdk-config.cmake
    aduagent-sdk-targets.cmake

/usr/share/aduagent-sdk/             # Tools and templates
    error_code_generator/
        generate_handler_errors.py
        error_code_template.h
    examples/
        minimal_handler/
        swupdate_handler_example/
    
/usr/share/doc/aduagent-sdk/         # Documentation
    README.md
    handler-development-guide.md
    api-reference.md
```

## Key SDK Components

### 1. Result and Error Code System

**File: `inc/aduc/sdk_result.h`**

```c
/**
 * @file sdk_result.h
 * @brief ADU Agent SDK result types and error code generation
 */

#ifndef ADUC_SDK_RESULT_H
#define ADUC_SDK_RESULT_H

#include <stdbool.h>
#include <stdint.h>

// Result type definition
typedef int32_t ADUC_Result_t;

typedef struct tagADUC_Result
{
    ADUC_Result_t ResultCode;
    ADUC_Result_t ExtendedResultCode;
} ADUC_Result;

// Success indicator macros
static inline bool IsAducResultCodeSuccess(const ADUC_Result_t resultCode)
{
    return (resultCode > 0);
}

static inline bool IsAducResultCodeFailure(const ADUC_Result_t resultCode)
{
    return (resultCode <= 0);
}

// Standard result codes
#define ADUC_Result_Failure 0
#define ADUC_Result_Success 1
#define ADUC_Result_Success_NoWork 2

// Extended result code generator
static inline ADUC_Result_t
MAKE_ADUC_EXTENDEDRESULTCODE(const int32_t facility, const int32_t component, const int32_t value)
{
    return ((facility & 0xF) << 0x1C) | ((component & 0xFF) << 0x14) | (value & 0xFFFFF);
}

// Macro for handlers to define their own extended result codes
#define MAKE_HANDLER_EXTENDEDRESULTCODE(facility, component, code) \
    MAKE_ADUC_EXTENDEDRESULTCODE(facility, component, code)

#endif // ADUC_SDK_RESULT_H
```

### 2. Workflow Access Interface

**File: `inc/aduc/sdk_workflow.h`**

```c
/**
 * @file sdk_workflow.h
 * @brief Workflow data access APIs for handlers
 */

#ifndef ADUC_SDK_WORKFLOW_H
#define ADUC_SDK_WORKFLOW_H

#include "sdk_result.h"
#include "sdk_types.h"

// Workflow handle (opaque pointer)
typedef void* ADUC_WorkflowHandle;

// Get workflow ID
const char* sdk_workflow_get_id(ADUC_WorkflowHandle handle);

// Get update type
const char* sdk_workflow_get_update_type(ADUC_WorkflowHandle handle);

// Get installed criteria
const char* sdk_workflow_get_installed_criteria(ADUC_WorkflowHandle handle);

// Get work folder
char* sdk_workflow_get_work_folder(ADUC_WorkflowHandle handle);

// Get update files count
size_t sdk_workflow_get_update_files_count(ADUC_WorkflowHandle handle);

// Get file entity by index
ADUC_Result sdk_workflow_get_update_file(
    ADUC_WorkflowHandle handle,
    size_t index,
    ADUC_FileEntity* fileEntity);

// Get handler property (string)
const char* sdk_workflow_get_handler_property_string(
    ADUC_WorkflowHandle handle,
    const char* propertyName);

// Free work folder string
void sdk_workflow_free_string(char* str);

#endif // ADUC_SDK_WORKFLOW_H
```

### 3. System Utilities

**File: `inc/aduc/sdk_system.h`**

```c
/**
 * @file sdk_system.h
 * @brief System utility functions for handlers
 */

#ifndef ADUC_SDK_SYSTEM_H
#define ADUC_SDK_SYSTEM_H

#include "sdk_result.h"
#include <stdbool.h>

// File operations
bool sdk_system_file_exists(const char* path);
ADUC_Result sdk_system_create_directory(const char* path);
ADUC_Result sdk_system_delete_file(const char* path);
ADUC_Result sdk_system_copy_file(const char* src, const char* dest);

// Environment variables
char* sdk_system_getenv(const char* name);
ADUC_Result sdk_system_setenv(const char* name, const char* value);

// U-Boot environment (for bootloader integration)
char* sdk_uboot_getenv(const char* name);
ADUC_Result sdk_uboot_setenv(const char* name, const char* value);
ADUC_Result sdk_uboot_saveenv(void);

#endif // ADUC_SDK_SYSTEM_H
```

### 4. Process Execution

**File: `inc/aduc/sdk_process.h`**

```c
/**
 * @file sdk_process.h
 * @brief Process execution utilities
 */

#ifndef ADUC_SDK_PROCESS_H
#define ADUC_SDK_PROCESS_H

#include "sdk_result.h"

// Process execution result
typedef struct tagSDK_ProcessResult
{
    int exitCode;
    char* stdOut;
    char* stdErr;
} SDK_ProcessResult;

// Execute command and capture output
ADUC_Result sdk_process_execute(
    const char* command,
    const char* const* args,
    const char* workingDir,
    SDK_ProcessResult* result);

// Free process result
void sdk_process_free_result(SDK_ProcessResult* result);

#endif // ADUC_SDK_PROCESS_H
```

### 5. Logging Interface

**File: `inc/aduc/sdk_logging.h`**

```c
/**
 * @file sdk_logging.h
 * @brief Logging interface for handlers
 */

#ifndef ADUC_SDK_LOGGING_H
#define ADUC_SDK_LOGGING_H

#include <stdbool.h>

// Log levels
typedef enum tagSDK_LogLevel
{
    SDK_LOG_ERROR = 0,
    SDK_LOG_WARN = 1,
    SDK_LOG_INFO = 2,
    SDK_LOG_DEBUG = 3
} SDK_LogLevel;

// Initialize logging for handler
bool sdk_logging_init(const char* handlerName);

// Log functions
void sdk_log(SDK_LogLevel level, const char* fmt, ...);

// Convenience macros
#define SDK_LOG_ERROR(...) sdk_log(SDK_LOG_ERROR, __VA_ARGS__)
#define SDK_LOG_WARN(...) sdk_log(SDK_LOG_WARN, __VA_ARGS__)
#define SDK_LOG_INFO(...) sdk_log(SDK_LOG_INFO, __VA_ARGS__)
#define SDK_LOG_DEBUG(...) sdk_log(SDK_LOG_DEBUG, __VA_ARGS__)

// Cleanup logging
void sdk_logging_uninit(void);

#endif // ADUC_SDK_LOGGING_H
```

## Handler-Specific Error Code Generation

### Process

Each handler will have its own error code definition file:

**Example: `swupdate_handler_v3_errors.json`**

```json
{
    "handler_name": "swupdate_handler_v3",
    "facility_code": 10,
    "component_code": 3,
    "errors": [
        {
            "name": "SWUPDATE_V3_ERC_SCRIPT_NOT_FOUND",
            "value": 1,
            "description": "SWUpdate script file not found"
        },
        {
            "name": "SWUPDATE_V3_ERC_SWU_FILE_NOT_FOUND",
            "value": 2,
            "description": "SWU update file not found"
        },
        {
            "name": "SWUPDATE_V3_ERC_UBOOT_ENV_READ_FAILED",
            "value": 3,
            "description": "Failed to read U-Boot environment variable"
        },
        {
            "name": "SWUPDATE_V3_ERC_UBOOT_ENV_WRITE_FAILED",
            "value": 4,
            "description": "Failed to write U-Boot environment variable"
        },
        {
            "name": "SWUPDATE_V3_ERC_BOOT_HEALTH_CHECK_FAILED",
            "value": 5,
            "description": "Boot health check failed"
        },
        {
            "name": "SWUPDATE_V3_ERC_PARTITION_SWITCH_FAILED",
            "value": 6,
            "description": "Failed to switch boot partition"
        },
        {
            "name": "SWUPDATE_V3_ERC_SWUPDATE_EXECUTION_FAILED",
            "value": 100,
            "description": "SWUpdate execution failed"
        }
    ]
}
```

### Generation Script

**File: `error_code_generator/generate_handler_errors.py`**

```python
#!/usr/bin/env python3
"""
Generate handler-specific error code header from JSON definition
"""

import argparse
import json

def generate_error_header(json_file, output_file):
    with open(json_file, 'r') as f:
        config = json.load(f)
    
    handler_name = config['handler_name']
    facility = config['facility_code']
    component = config['component_code']
    errors = config['errors']
    
    # Generate header content
    header = f"""
/**
 * @file {handler_name}_errors.h
 * @brief Error codes for {handler_name}
 * Generated from {json_file}
 */

#ifndef {handler_name.upper()}_ERRORS_H
#define {handler_name.upper()}_ERRORS_H

#include <aduc/sdk_result.h>

// Facility and component codes
#define {handler_name.upper()}_FACILITY {facility}
#define {handler_name.upper()}_COMPONENT {component}

// Error code generator macro
#define MAKE_{handler_name.upper()}_ERC(code) \\
    MAKE_ADUC_EXTENDEDRESULTCODE({handler_name.upper()}_FACILITY, {handler_name.upper()}_COMPONENT, code)

// Error code definitions
"""
    
    for error in errors:
        name = error['name']
        value = error['value']
        desc = error['description']
        header += f"#define {name} MAKE_{handler_name.upper()}_ERC({value}) // {desc}\n"
    
    header += f"\n#endif // {handler_name.upper()}_ERRORS_H\n"
    
    with open(output_file, 'w') as f:
        f.write(header)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("input", help="Input JSON file")
    parser.add_argument("output", help="Output header file")
    args = parser.parse_args()
    
    generate_error_header(args.input, args.output)
```

## SWUpdate Handler V3 Architecture

### Key Features

1. **U-Boot Integration**: 
   - Uses standard U-Boot environment variables instead of rpipart
   - Variables: `boot_partition`, `upgrade_available`, `bootcount`, etc.

2. **Boot Health Check**:
   - Implements boot health verification after update
   - Rollback mechanism if health check fails
   - Configurable health check scripts

3. **Self-Contained Error Codes**:
   - Handler-specific error code definitions
   - Clear error reporting with extended result codes

4. **Minimal Dependencies**:
   - Only depends on aduagent-sdk
   - Can be built independently

### File Structure

```
/src/extensions/step_handlers/swupdate_handler_v3/
├── CMakeLists.txt
├── README.md
├── swupdate_handler_v3_errors.json    # Error code definitions
├── inc/
│   ├── swupdate_handler_v3.hpp
│   └── swupdate_handler_v3_errors.h   # Generated error codes
├── src/
│   ├── handler_create.cpp
│   ├── swupdate_handler_v3.cpp
│   ├── uboot_manager.cpp              # U-Boot env management
│   └── boot_health_checker.cpp        # Boot health verification
├── scripts/
│   ├── swupdate-wrapper.sh            # Main execution script
│   └── boot-health-check.sh           # Health check script
└── tests/
    └── unit_tests/
```

### Handler Configuration

**File: `swupdate-handler-v3-config.json`**

```json
{
    "description": "SWUpdate Handler V3 Configuration",
    "uboot": {
        "boot_partition_var": "boot_partition",
        "upgrade_available_var": "upgrade_available",
        "bootcount_var": "bootcount",
        "bootlimit": 3
    },
    "health_check": {
        "enabled": true,
        "script": "/usr/lib/adu/boot-health-check.sh",
        "timeout_seconds": 120
    },
    "swupdate": {
        "binary_path": "/usr/bin/swupdate",
        "default_args": ["-v"]
    }
}
```

### U-Boot Environment Variables

The handler will use standard U-Boot variables:

- `boot_partition`: Current boot partition (0 or 1)
- `upgrade_available`: Flag indicating new update (0 or 1)
- `bootcount`: Number of boot attempts
- `bootlimit`: Maximum boot attempts before rollback
- `boot_successful`: Flag set after successful health check

### Boot Health Check Flow

```
1. Update installed to inactive partition
2. Set upgrade_available=1
3. Set bootcount=0
4. Set boot_partition to new partition
5. Reboot
6. After boot:
   - Increment bootcount
   - If bootcount > bootlimit: rollback
   - Run health check script
   - If success: Set upgrade_available=0, boot_successful=1
   - If fail: Continue boot attempts or rollback
```

## CMake Integration

### SDK CMakeLists.txt

**File: `/src/aduagent-sdk/CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.5)
project(aduagent-sdk VERSION 1.0.0)

# SDK library sources
set(SDK_SOURCES
    src/sdk_result.c
    src/sdk_logging.c
    src/sdk_workflow.c
    src/sdk_process.c
    src/sdk_string.c
    src/sdk_system.c
    src/sdk_config.c
)

# SDK headers
set(SDK_HEADERS
    inc/aduc/sdk_result.h
    inc/aduc/sdk_types.h
    inc/aduc/sdk_logging.h
    inc/aduc/sdk_workflow.h
    inc/aduc/sdk_process.h
    inc/aduc/sdk_string.h
    inc/aduc/sdk_system.h
    inc/aduc/sdk_config.h
    inc/aduc/sdk_content_handler.h
)

# Create SDK library
add_library(aduagent-sdk SHARED ${SDK_SOURCES})
add_library(aduagent-sdk-static STATIC ${SDK_SOURCES})

target_include_directories(aduagent-sdk
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
        $<INSTALL_INTERFACE:include>
)

target_include_directories(aduagent-sdk-static
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
        $<INSTALL_INTERFACE:include>
)

# Installation
install(TARGETS aduagent-sdk aduagent-sdk-static
    EXPORT aduagent-sdk-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY inc/aduc
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

install(FILES error_code_generator/generate_handler_errors.py
    DESTINATION share/aduagent-sdk/error_code_generator
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

# CMake config files
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/aduagent-sdk-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(EXPORT aduagent-sdk-targets
    FILE aduagent-sdk-targets.cmake
    NAMESPACE aduagent::
    DESTINATION lib/cmake/aduagent-sdk
)

configure_file(cmake/aduagent-sdk-config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/aduagent-sdk-config.cmake"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/aduagent-sdk-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/aduagent-sdk-config-version.cmake"
    DESTINATION lib/cmake/aduagent-sdk
)
```

### Handler CMakeLists.txt (using SDK)

**File: `/src/extensions/step_handlers/swupdate_handler_v3/CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.5)
project(swupdate_handler_v3)

# Find SDK
find_package(aduagent-sdk REQUIRED)

# Generate error codes
set(ERROR_CODE_JSON ${CMAKE_CURRENT_SOURCE_DIR}/swupdate_handler_v3_errors.json)
set(ERROR_CODE_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/inc/swupdate_handler_v3_errors.h)

add_custom_command(
    OUTPUT ${ERROR_CODE_HEADER}
    COMMAND python3 /usr/share/aduagent-sdk/error_code_generator/generate_handler_errors.py
            ${ERROR_CODE_JSON} ${ERROR_CODE_HEADER}
    DEPENDS ${ERROR_CODE_JSON}
    COMMENT "Generating error codes for swupdate_handler_v3"
)

add_custom_target(generate_swupdate_v3_errors DEPENDS ${ERROR_CODE_HEADER})

# Handler library
set(target_name microsoft_swupdate_3)
add_library(${target_name} MODULE
    src/handler_create.cpp
    src/swupdate_handler_v3.cpp
    src/uboot_manager.cpp
    src/boot_health_checker.cpp
)

add_dependencies(${target_name} generate_swupdate_v3_errors)

target_include_directories(${target_name}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

# Link only against SDK
target_link_libraries(${target_name}
    PRIVATE
        aduagent::aduagent-sdk
)

# Install handler
install(TARGETS ${target_name}
    LIBRARY DESTINATION lib/adu/extensions
)

# Install scripts
install(PROGRAMS
    scripts/swupdate-wrapper.sh
    scripts/boot-health-check.sh
    DESTINATION lib/adu/scripts
)
```

## Yocto/BitBake Integration

### deviceupdate-agent-dev Recipe

**File: `recipes-azure/deviceupdate-agent-dev/deviceupdate-agent-dev_1.0.0.bb`**

```bitbake
SUMMARY = "Azure Device Update Agent SDK for handler development"
DESCRIPTION = "Development package providing headers and libraries for building custom ADU handlers"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = ""
RDEPENDS_${PN}-dev = "python3 cmake"

SRC_URI = "git://github.com/Azure/iot-hub-device-update.git;protocol=https;branch=main"
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git"

inherit cmake

EXTRA_OECMAKE = " \
    -DADUC_BUILD_SDK_ONLY=ON \
    -DCMAKE_BUILD_TYPE=Release \
"

do_install_append() {
    # Install error code generator scripts
    install -d ${D}${datadir}/aduagent-sdk/error_code_generator
    install -m 0755 ${S}/src/aduagent-sdk/error_code_generator/*.py ${D}${datadir}/aduagent-sdk/error_code_generator/
    
    # Install documentation
    install -d ${D}${docdir}/aduagent-sdk
    install -m 0644 ${S}/docs/agent-reference/aduagent-sdk-*.md ${D}${docdir}/aduagent-sdk/
}

# This is a development package
ALLOW_EMPTY_${PN} = "1"
FILES_${PN}-dev = " \
    ${includedir}/aduc/* \
    ${libdir}/libaduagent-sdk.* \
    ${libdir}/cmake/aduagent-sdk/* \
    ${datadir}/aduagent-sdk/* \
    ${docdir}/aduagent-sdk/* \
"

BBCLASSEXTEND = "native nativesdk"
```

### Handler Recipe (Independent Build)

**File: `recipes-azure/swupdate-handler-v3/swupdate-handler-v3_1.0.0.bb`**

```bitbake
SUMMARY = "SWUpdate Handler V3 for Azure Device Update"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = "deviceupdate-agent-dev swupdate"
RDEPENDS_${PN} = "swupdate deviceupdate-agent"

SRC_URI = " \
    file://src/ \
    file://inc/ \
    file://CMakeLists.txt \
    file://swupdate_handler_v3_errors.json \
    file://scripts/ \
"

S = "${WORKDIR}"

inherit cmake

EXTRA_OECMAKE = " \
    -DCMAKE_BUILD_TYPE=Release \
"

do_install_append() {
    # Install configuration
    install -d ${D}${sysconfdir}/adu
    install -m 0644 ${WORKDIR}/swupdate-handler-v3-config.json ${D}${sysconfdir}/adu/
    
    # Install scripts
    install -d ${D}${libdir}/adu/scripts
    install -m 0755 ${WORKDIR}/scripts/*.sh ${D}${libdir}/adu/scripts/
}

FILES_${PN} += " \
    ${libdir}/adu/extensions/*.so \
    ${libdir}/adu/scripts/* \
    ${sysconfdir}/adu/*.json \
"
```

## Implementation Roadmap

### Phase 1: SDK Foundation (Week 1-2)

1. Create `/src/aduagent-sdk` directory structure
2. Define SDK header interfaces
3. Implement SDK wrapper functions
4. Create error code generation scripts
5. Write SDK CMake configuration
6. Create SDK documentation

**Deliverables:**
- SDK headers and implementation
- CMake build system for SDK
- Error code generator tool
- Basic SDK documentation

### Phase 2: Refactor Agent Dependencies (Week 3)

1. Identify which agent utilities are used by handlers
2. Create SDK wrappers for these utilities
3. Update existing handlers to optionally use SDK (backward compatible)
4. Test that SDK provides all necessary functionality

**Deliverables:**
- Complete SDK implementation
- Unit tests for SDK
- Migration guide for existing handlers

### Phase 3: Yocto Packaging (Week 4)

1. Create `deviceupdate-agent-dev` BitBake recipe
2. Test SDK package installation
3. Verify SDK headers and libraries are accessible
4. Create example handler using only SDK

**Deliverables:**
- `deviceupdate-agent-dev.bb` recipe
- Tested SDK package
- Example handler project

### Phase 4: SWUpdate Handler V3 Implementation (Week 5-6)

1. Create handler directory structure
2. Define error codes in JSON
3. Implement U-Boot environment manager
4. Implement boot health checker
5. Implement main handler logic
6. Create wrapper scripts
7. Write unit and integration tests

**Deliverables:**
- Complete swupdate-handler-v3 implementation
- Handler BitBake recipe
- Test suite
- Documentation

### Phase 5: Testing and Documentation (Week 7)

1. End-to-end testing on target hardware
2. Verify independent build capability
3. Create comprehensive documentation
4. Write handler development tutorial

**Deliverables:**
- Tested handler on target device
- Complete documentation
- Developer tutorial
- Best practices guide

## Benefits

1. **For Device Builders:**
   - Build custom handlers without full agent source
   - Clear, stable API to develop against
   - Faster development and testing cycles
   - Independent versioning of handlers

2. **For ADU Team:**
   - Clear separation of concerns
   - Easier to maintain public API stability
   - Better testability
   - Encourages community contributions

3. **For System:**
   - Reduced build times for handlers
   - Smaller dependencies
   - Better modularity
   - Easier to distribute handler development capability

## Open Questions

1. **Versioning Strategy:**
   - How do we handle SDK version compatibility?
   - Should handlers specify minimum SDK version?

2. **ABI Stability:**
   - What guarantees do we make about binary compatibility?
   - When can we break ABI?

3. **Distribution:**
   - Should SDK be a separate repository?
   - How do we handle SDK updates?

4. **Testing:**
   - How do we test SDK independently?
   - What's the testing strategy for SDK compatibility?

## Next Steps

1. Review and approve this design document
2. Create work items for Phase 1
3. Set up SDK development branch
4. Begin implementation

---

**Document Version:** 1.0  
**Date:** January 4, 2026  
**Author:** ADU Development Team  
**Status:** Draft for Review
