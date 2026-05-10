# ADU Gen2 Log Viewer

A Python CLI tool for viewing, filtering, and analyzing ADU Gen2 agent logs.

## Features

- Parse both **binary** and **text** log formats (auto-detected)
- Filter by component, severity level, event ID range, time range, or keyword
- Decode result codes inline (maps hex codes to symbolic names)
- Colorized terminal output
- Statistics mode (event counts, error rates, component breakdown)
- Follow mode (`tail -f` equivalent)
- JSON output mode (for piping to other tools)
- Grep with context lines

## Requirements

- Python 3.6+ (stdlib only, no external dependencies)

## Usage

```bash
# View all logs
python log_viewer.py /var/log/adu-gen2.log

# Filter by component and level
python log_viewer.py --component workflow --level error /var/log/adu-gen2.log

# Multiple components
python log_viewer.py --component workflow,comm,download --level warn log.txt

# Filter by time range
python log_viewer.py --since "2024-01-15 10:00:00" --until "2024-01-15 11:00:00" log.bin

# Filter by event ID or range
python log_viewer.py --event-id 2001 log.txt
python log_viewer.py --event-id 2001-2020 log.txt

# Show statistics
python log_viewer.py --stats /var/log/adu-gen2.log

# Follow mode (tail -f)
python log_viewer.py --follow /var/log/adu-gen2.log

# Follow with filters
python log_viewer.py --follow --component workflow --level error /var/log/adu-gen2.log

# Decode result codes inline
python log_viewer.py --decode-rc /var/log/adu-gen2.log

# JSON output (for piping to jq or other tools)
python log_viewer.py --json /var/log/adu-gen2.log

# Search with context
python log_viewer.py --grep "failed" --context 3 /var/log/adu-gen2.log

# Count matching entries
python log_viewer.py --component workflow --level error --count log.txt

# Disable color (useful for piping)
python log_viewer.py --no-color /var/log/adu-gen2.log | less
```

## Log Formats

### Text Format

```
2024-01-15T10:30:00.123Z [INFO] [workflow] (2001) Workflow execution started: wf-001
```

Fields: `timestamp [LEVEL] [component] (event_id) message`

### Binary Format

```
[timestamp:8 bytes][level:1 byte][component_len:1 byte][component:N bytes][event_id:2 bytes][msg_len:2 bytes][message:N bytes]
```

- `timestamp`: uint64, milliseconds since Unix epoch (little-endian)
- `level`: uint8 (0=FATAL, 1=ERROR, 2=WARN, 3=INFO, 4=DEBUG, 5=TRACE)
- `component_len`: uint8, length of component name
- `component`: UTF-8 component name
- `event_id`: uint16, little-endian
- `msg_len`: uint16, little-endian
- `message`: UTF-8 message text

The viewer auto-detects format, or you can force it with `--format text|binary`.

## Event ID Ranges

| Range     | Component      |
|-----------|----------------|
| 1xxx      | Agent Core     |
| 2xxx      | Workflow Engine |
| 3xxx      | Extension Framework |
| 4xxx      | Communication  |
| 5xxx      | Download       |
| 6xxx      | Security       |
| 7xxx      | DAG Engine     |
| 8xxx      | Script Handler |
| 9xxx      | APT Handler    |
| 0xAxxx    | SWUpdate Handler |

## Colorization

| Level   | Color  |
|---------|--------|
| FATAL   | Bold Red |
| ERROR   | Red    |
| WARN    | Yellow |
| INFO    | Green  |
| DEBUG   | Cyan   |
| TRACE   | Gray   |

Disable with `--no-color` or set the `NO_COLOR` environment variable.

## Statistics Output

```
Log Analysis Report
══════════════════════════════════════════════════════
Total entries:     1,234
Time span:         2024-01-15 10:00:00 → 10:05:32 (5m 32s)

By Level:
  ERROR       12 ( 0.97%)  ████
  WARN        45 ( 3.6%)   ████████████
  INFO       890 (72.1%)   ████████████████████████████████████████
  DEBUG      287 (23.3%)   ███████████████████████

By Component:
  workflow      456 (37.0%)
  extension    234 (19.0%)
  comm         198 (16.0%)
  agent        156 (12.6%)
  download     120 (9.7%)
  security      70 (5.7%)

Top Errors:
  4002: COMM_4002 (5 occurrences)
  5004: DOWNLOAD_5004 (3 occurrences)
  2012: WORKFLOW_2012 (2 occurrences)
```

## Integration with Log String Package

This tool mirrors the event ID registry defined in:
- `src/extension_sdk/inc/aduc/log_strings.h` — Event ID definitions
- `src/extension_sdk/src/log_strings.c` — Format string table

When decoding binary logs, the viewer uses its built-in Python copy of the
string table to resolve event IDs to human-readable messages.

## Result Code Decoding

When `--decode-rc` is enabled, hexadecimal result codes in log messages are
annotated with their symbolic names:

```
Before: Download failed: file.swu (rc=0x80048001, http_status=404)
After:  Download failed: file.swu (rc=0x80048001 [ADUC_E_DOWNLOAD_FAILURE], http_status=404)
```
