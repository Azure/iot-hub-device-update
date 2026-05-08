# ADU Gen2 Result Code Decoder

A Python CLI tool that decodes ADU Gen2 numeric result codes (RC/ERC) into
human-readable diagnostics.

## Requirements

- Python 3.6+ (stdlib only, no external dependencies)

## Usage

```bash
# Decode a single hex code
python rc_decoder.py 0x01010001
# => ADUC_RC_AGENT_CONFIG_FILE_NOT_FOUND
#    Facility: AGENT (0x01), Category: CONFIG (0x01), Specific: 0x0001

# Decode a decimal code
python rc_decoder.py 16842753

# Decode multiple codes
python rc_decoder.py 0x01010001 0x02030007 0x04100003

# List all codes for a facility
python rc_decoder.py --list --facility AGENT

# List all codes for a specific category within a facility
python rc_decoder.py --list --facility EXTENSION --category SCRIPT

# Search codes by keyword
python rc_decoder.py --search "timeout"
python rc_decoder.py --search "certificate"

# Decode codes found in a log file
python rc_decoder.py --parse-log /var/log/adu-gen2.log
```

## Result Code Layout

```
  31       24 23      16 15              0
  ┌─────────┬──────────┬────────────────┐
  │ facility │ category │    specific    │
  │   (8b)   │   (8b)   │    (16b)      │
  └─────────┴──────────┴────────────────┘
```

## Facilities

| Code | Name       | Description                        |
|------|------------|------------------------------------|
| 0x01 | AGENT      | Agent lifecycle and core           |
| 0x02 | DOWNLOAD   | Download manager                   |
| 0x03 | WORKFLOW   | Workflow engine and orchestration   |
| 0x04 | EXTENSION  | Extension loader and framework     |
| 0x05 | COMM       | Communication layer                |
| 0x06 | SECURITY   | Security and crypto operations     |

## Categories

| Code | Name       | Description                        |
|------|------------|------------------------------------|
| 0x01 | CONFIG     | Configuration errors               |
| 0x02 | AUTH       | Authentication/authorization       |
| 0x03 | IO         | I/O operations                     |
| 0x04 | PROTOCOL   | Protocol-level errors              |
| 0x05 | RESOURCE   | Resource exhaustion                |
| 0x06 | STATE      | State machine violations           |
| 0x07 | DAG        | DAG engine specific                |
| 0x10 | SCRIPT     | Script handler specific            |
| 0x11 | APT        | APT handler specific               |
| 0x12 | SWUPDATE   | SWUpdate handler specific          |

## Related Files

- Header: `src/extension_sdk/inc/aduc/result_codes.h` — C macro definitions
- Compat: `src/extension_sdk/inc/aduc/erc_compat.h` — Gen1 ERC mapping
