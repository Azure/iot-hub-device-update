# Diagnostics & Troubleshooting

## Result Code System

### Layout

Gen2 result codes are 32-bit structured values:

```
┌──────────┬──────────┬─────────────────────┐
│ Facility │ Category │      Specific       │
│  8 bits  │  8 bits  │      16 bits        │
└──────────┴──────────┴─────────────────────┘
  Bits 31-24  Bits 23-16     Bits 15-0
```

**Extraction macros:**

```c
#define ADUC_RC_GET_FACILITY(rc)  (((rc).code >> 24) & 0xFF)
#define ADUC_RC_GET_CATEGORY(rc)  (((rc).code >> 16) & 0xFF)
#define ADUC_RC_GET_SPECIFIC(rc)  ((rc).code & 0xFFFF)
```

### Facilities

| Code | Facility | Description |
|------|----------|-------------|
| 0x01 | AGENT | Agent core operations |
| 0x02 | DOWNLOAD | Content download and I/O |
| 0x03 | WORKFLOW | Workflow engine and orchestration |
| 0x04 | EXTENSION | Extension loading and execution |
| 0x05 | COMM | Communication providers |
| 0x06 | SECURITY | Certificates, keys, authentication |

### Categories

| Code | Category | Used By |
|------|----------|---------|
| 0x01 | CONFIG | All facilities |
| 0x02 | AUTH | COMM, SECURITY |
| 0x03 | IO | DOWNLOAD, AGENT |
| 0x04 | PROTOCOL | COMM |
| 0x05 | RESOURCE | All facilities |
| 0x06 | STATE | WORKFLOW |
| 0x07 | DAG | WORKFLOW |
| 0x10 | SCRIPT | EXTENSION |
| 0x11 | APT | EXTENSION |
| 0x12 | SWUPDATE | EXTENSION |

### Common Result Codes

| Code | Macro | Meaning |
|------|-------|---------|
| `0x01010001` | `ADUC_RC_AGENT_CONFIG_FILE_NOT_FOUND` | Config file missing |
| `0x02030003` | `ADUC_RC_DOWNLOAD_IO_HASH_MISMATCH` | Download integrity check failed |
| `0x04100002` | `ADUC_RC_EXTENSION_SCRIPT_TIMEOUT` | Script exceeded time limit |
| `0x05020001` | `ADUC_RC_COMM_AUTH_FAILURE` | Authentication to service failed |
| `0x06020001` | `ADUC_RC_SECURITY_AUTH_CERT_EXPIRED` | Device certificate expired |

### Gen1 ERC Compatibility

The `erc_compat.h` header provides bidirectional mapping between Gen2 structured codes and Gen1 Extended Result Codes:

```c
int32_t erc = ADUC_MapToExtendedResultCode(gen2_result);    // Gen2 → Gen1
ADUC_Result2 rc = ADUC_MapFromExtendedResultCode(erc);      // Gen1 → Gen2
```

**Gen1 ERC format:** `0x{component:4}{facility:4}{code:24}`

---

## Using rc_decoder.py

Location: `tools/rc_decoder/rc_decoder.py`

### Decode a Result Code

```bash
# From hex
python3 tools/rc_decoder/rc_decoder.py 0x01010001

# From decimal
python3 tools/rc_decoder/rc_decoder.py 16842753
```

**Output:**
```
=== Result Code: 0x01010001 ===
Facility: AGENT (0x01)
Category: CONFIG (0x01)
Specific: 0x0001
ADUC_RC_AGENT_CONFIG_FILE_NOT_FOUND
Configuration file not found
```

### List All Codes

```bash
# All codes
python3 tools/rc_decoder/rc_decoder.py --list

# Filter by facility
python3 tools/rc_decoder/rc_decoder.py --list --facility AGENT

# Filter by category
python3 tools/rc_decoder/rc_decoder.py --list --category IO
```

### Search by Keyword

```bash
python3 tools/rc_decoder/rc_decoder.py --search "timeout"
python3 tools/rc_decoder/rc_decoder.py --search "certificate"
```

### Parse Log File for Result Codes

```bash
python3 tools/rc_decoder/rc_decoder.py --parse-log /var/log/adu/adu-gen2.log
```

Scans the log for embedded result codes and decodes each one in context.

---

## Using log_viewer.py

Location: `tools/log_viewer/log_viewer.py`

### Log Entry Format

**Text format:**
```
2024-01-15T10:30:00.123Z [INFO] [workflow] (2001) Workflow execution started
```

**Binary format:** Compact binary encoding (8-byte timestamp + level + component + event_id + message)

### Basic Usage

```bash
# View log file
python3 tools/log_viewer/log_viewer.py --file /var/log/adu/adu-gen2.log

# Filter by level
python3 tools/log_viewer/log_viewer.py --file adu-gen2.log --level ERROR

# Filter by component
python3 tools/log_viewer/log_viewer.py --file adu-gen2.log --filter workflow

# Follow mode (like tail -f)
python3 tools/log_viewer/log_viewer.py --file adu-gen2.log --follow
```

### Statistics

```bash
python3 tools/log_viewer/log_viewer.py --file adu-gen2.log --stats
```

Shows counts by level, component, and most frequent event IDs.

### Log Levels

| Value | Level | Description |
|-------|-------|-------------|
| 0 | FATAL | Unrecoverable errors |
| 1 | ERROR | Operation failures |
| 2 | WARN | Potential issues |
| 3 | INFO | Key operational events |
| 4 | NOTICE | Normal but significant |
| 5 | DEBUG | Detailed diagnostic info |
| 6 | TRACE | Verbose execution trace |

---

## Event ID Reference

Event IDs are uint16 values organized by component. Use them to quickly identify log sources.

### Ranges

| Component | Range | Example |
|-----------|-------|---------|
| Agent Core | 1001–1049 | `1001` = AGENT_STARTING |
| Workflow Engine | 2001–2040 | `2001` = WF_EXECUTE_START |
| Extension Framework | 3001–3031 | `3002` = EXT_LOAD_SUCCESS |
| Communication | 4001–4022 | `4001` = COMM_CONNECT_START |
| Download | 5001–5021 | `5001` = DL_START |
| Security | 6001–6021 | `6001` = SEC_CERT_LOAD |
| DAG Engine | 7001–7032 | `7002` = DAG_CYCLE_DETECTED |
| Script Handler | 8001–8040 | `8010` = SCRIPT_EXECUTE_START |
| APT Handler | 9001–9040 | `9001` = APT_INSTALL_START |
| SWUpdate Handler | 0xA001–0xA030 | `0xA012` = SWU_EXECUTE_COMPLETE |

### Key Event IDs for Troubleshooting

| ID | Event | What to Check |
|----|-------|---------------|
| 1001 | Agent starting | Config file path, permissions |
| 2001 | Workflow start | Manifest parsing, step count |
| 3001 | Extension load attempt | .so path, dlopen errors |
| 4001 | Connection attempt | Endpoint URL, network, certs |
| 7002 | DAG cycle detected | Circular `requires` dependencies |
| 7010 | DAG step skipped | Upstream failure cascade |
| 8010 | Script execution | Script path, permissions, timeout |

### Programmatic Access

```c
#include <aduc/log_strings.h>

const char* fmt = ADUC_LogString_GetFormat(2001);       // "Workflow execution started: %s"
const char* comp = ADUC_LogString_GetComponent(2001);   // "workflow"
size_t count = ADUC_LogString_GetCount();               // Total registered strings (88)
```

---

## Common Troubleshooting

### Agent won't start

1. Check config file exists and is valid TOML
2. Decode result code: `python3 tools/rc_decoder/rc_decoder.py <code>`
3. Check log: `python3 tools/log_viewer/log_viewer.py --file adu-gen2.log --level ERROR`

### Extension not loading

Look for event ID 3001 (load attempt) and 3003 (load failure):
```bash
python3 tools/log_viewer/log_viewer.py --file adu-gen2.log --filter extension
```

Common causes: missing .so file, wrong search path, missing shared library dependencies (`ldd extension.so`).

### DAG step stuck or skipped

Look for event IDs 7001–7032:
```bash
python3 tools/log_viewer/log_viewer.py --file adu-gen2.log --filter dag
```

Check: circular dependencies (7002), upstream failures (7010), missing step IDs in `requires`.

### Script handler failures

```bash
python3 tools/log_viewer/log_viewer.py --file adu-gen2.log --filter script
```

Check script exit codes: `1`=fail, `2`=retry (agent will retry), `3`=reboot needed.
