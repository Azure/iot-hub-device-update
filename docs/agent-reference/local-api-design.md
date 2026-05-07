# Agent Local API — Design Document

## Overview

The **Agent Local API** (`localapi`) is a cross-platform IPC mechanism that enables
external processes on the device to query the Device Update agent's status and send
control commands. It replaces the legacy FIFO-based service API with a secure,
bidirectional, connection-oriented transport.

## Motivation

The previous implementation (`apisvc`) used Linux named FIFOs (`mkfifo`) which:
- Only works on Linux with native filesystem (not NTFS, not Windows)
- Requires two separate FIFOs per request (request + response) creating race conditions
- Provides no peer authentication beyond file permissions
- Has no rate limiting or DoS protection

## Architecture

```
┌──────────────────┐          ┌────────────────────────┐
│  Client Process  │          │  Device Update Agent   │
│                  │          │                        │
│  aducsdk.h API   │◄────────►│  localapi_server.c     │
│  (GetStatus())   │   IPC    │  (listener thread)     │
└────────┬─────────┘          └───────────┬────────────┘
         │                                │
    ┌────┴─────────────────────────────────┴────┐
    │          ipc_transport.h (PAL)            │
    ├───────────────────┬───────────────────────┤
    │   Linux           │   Windows             │
    │   AF_UNIX socket  │   Named Pipe          │
    │   SO_PEERCRED     │   Pipe ACL + Token    │
    └───────────────────┴───────────────────────┘
```

## Endpoints

| Platform | Default Endpoint |
|----------|-----------------|
| Linux    | `/var/lib/adu/api/localapi.sock` |
| Windows  | `\\.\pipe\adu-agent-localapi` |

## Wire Protocol

All messages use a fixed 6-byte header followed by optional payload:

### Request
```
┌──────────┬──────────┬──────────┬─────────────────┐
│ ver (u16)│ type(u16)│ len(u16) │ payload (0-4090)│
└──────────┴──────────┴──────────┴─────────────────┘
```

### Response
```
┌──────────┬────────────┬──────────┬─────────────────┐
│ ver (u16)│ status(u16)│ len(u16) │ payload (0-4090)│
└──────────┴────────────┴──────────┴─────────────────┘
```

### Request Types

| Type | Value | Permission Required | Description |
|------|-------|-------------------|-------------|
| GET_STATUS | 0x01 | READ_STATUS | Get current agent status |
| PAUSE | 0x02 | PAUSE_RESUME | Pause update processing |
| RESUME | 0x03 | PAUSE_RESUME | Resume update processing |
| CANCEL | 0x04 | CANCEL | Cancel current operation |
| FORCE_CHECK | 0x05 | FORCE_CHECK | Force update check |

### Response Status Codes

| Code | Meaning |
|------|---------|
| 0-9 | Agent status (same as `ADUC_ServiceStatus` enum) |
| 400 | Bad request (invalid version or type) |
| 403 | Forbidden (insufficient permissions) |
| 429 | Too many requests (rate limited) |
| 501 | Not implemented |

## Security Model

### Access Control

| Layer | Linux | Windows |
|-------|-------|---------|
| Transport | Socket file permissions `0660` | Named pipe DACL (SDDL) |
| Identity | `SO_PEERCRED` (kernel-guaranteed UID/GID) | `GetNamedPipeClientProcessId()` + token |
| Authorization | UID/GID group membership check | Token SID / elevation check |

### Permission Levels

| Permission | Linux | Windows |
|-----------|-------|---------|
| READ_STATUS | Members of `adu` group | Any authenticated pipe client |
| PAUSE_RESUME | Members of `adu-admin` group | Elevated processes |
| CANCEL | Members of `adu-admin` group | Elevated processes |
| FORCE_CHECK | root only (UID 0) | SYSTEM or elevated admin |

### Rate Limiting

- 10 requests/second per peer (configurable)
- Maximum 5 concurrent connections (configurable)
- Automatic window reset after 1 second of inactivity

### DoS Protection

- Receive timeout: 5 seconds (auto-disconnect slow clients)
- Send timeout: 5 seconds (prevent write blocking)
- Connection limit enforcement before processing any data

## Source Files

```
src/libaducpal/
├── inc/aduc/ipc_transport.h          # Platform-agnostic transport interface
├── src/ipc_transport_linux.c         # Unix domain socket implementation
└── src/ipc_transport_win32.c         # Named pipe implementation

src/localapi/
├── inc/aduc/localapi.h               # Public server API
├── src/localapi_server.c             # Server implementation
├── src/localapi_security.h           # Security internals
├── src/localapi_security.c           # Auth + rate limiting
└── tests/
    ├── ipc_transport_ut.cpp          # Transport layer tests
    └── localapi_ut.cpp               # Server integration tests
```

## Usage

### Agent Side (Server)

```c
#include "aduc/localapi.h"

// Start with defaults
localapi_init(NULL);

// Or with custom config
LocalApiConfig config = {
    .endpoint = "/custom/path.sock",
    .maxConnections = 10,
    .maxRequestsPerSecond = 20,
    .timeoutMs = 3000,
};
localapi_init(&config);

// Shutdown
localapi_uninit();
```

### Client Side (SDK)

```c
#include "aduc/aducsdk.h"

// Same API as before — implementation uses localapi transport
ADUC_ServiceStatus status = GetAduServiceStatus();
printf("Agent status: %s\n", ADUC_ServiceStatusToString(status));
```

## Migration from Gen1

The public SDK API (`GetAduServiceStatus()`) remains unchanged. Internal transport
is replaced transparently. The FIFO-based code is retained as dead code for reference
but is no longer compiled by default.

## Testing

All tests use temporary endpoints (`/tmp/adu-localapi-test.sock` on Linux,
`\\.\pipe\adu-localapi-test` on Windows) and do not require any system services
or special filesystem support. Tests verify:

1. Server lifecycle (create/close)
2. Connection timeout behavior
3. Client/server bidirectional communication
4. Peer credential retrieval
5. Full request/response protocol flow
6. Invalid version rejection
7. Error handling (null args, double init, etc.)
