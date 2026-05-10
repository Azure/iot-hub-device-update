#!/usr/bin/env python3
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
# log_viewer.py - ADU Gen2 Agent Log Viewer/Processor
#
# A CLI tool for viewing, filtering, and analyzing ADU Gen2 agent logs.
# Supports both binary and text log formats with colorized output,
# filtering, statistics, and follow mode.

"""ADU Gen2 Agent Log Viewer/Processor."""

import argparse
import json
import os
import re
import struct
import sys
import time
from collections import Counter, OrderedDict
from datetime import datetime, timedelta, timezone

# ─── Event ID to Component/Format string table ───────────────────────────────
# Mirrors the C string table in log_strings.c for offline decoding.

EVENT_TABLE = {
    # Agent Core (1xxx)
    1001: ("agent", "Agent starting (version={}, device={})"),
    1002: ("agent", "Agent started successfully (pid={})"),
    1003: ("agent", "Agent shutting down (reason={})"),
    1004: ("agent", "Agent shutdown complete (uptime={} seconds)"),
    1010: ("agent", "Configuration loaded from {}"),
    1011: ("agent", "Configuration error: {} (file={}, line={})"),
    1020: ("agent", "Extension loaded: {} (version={})"),
    1021: ("agent", "Extension load failed: {} (rc={})"),
    1022: ("agent", "Scanning extension directory: {}"),
    1030: ("agent", "Running in single-deployment mode"),
    1031: ("agent", "Running in daemon mode (poll_interval={} seconds)"),
    1040: ("agent", "Starting deployment poll cycle #{}"),
    1041: ("agent", "Poll complete: no pending deployment"),
    1042: ("agent", "Poll found deployment: {} (provider={}, name={}, version={})"),
    # Workflow Engine (2xxx)
    2001: ("workflow", "Workflow execution started: {} (steps={})"),
    2002: ("workflow", "Workflow completed successfully: {} (duration={} ms)"),
    2003: ("workflow", "Workflow failed: {} (rc={}, step={})"),
    2004: ("workflow", "Workflow cancelled: {} (by={})"),
    2010: ("workflow", "Step started: {} (index={}, handler={})"),
    2011: ("workflow", "Step completed: {} (index={}, duration={} ms)"),
    2012: ("workflow", "Step failed: {} (index={}, rc={}, detail={})"),
    2013: ("workflow", "Step skipped: {} (index={}, reason={})"),
    2020: ("workflow", "No handler found for step type: {}"),
    2030: ("workflow", "Cancel requested for workflow: {} (source={})"),
    2040: ("workflow", "Workflow progress: {} ({}% complete, step {}/{})"),
    # Extension Framework (3xxx)
    3001: ("extension", "Loading extension: {} from {}"),
    3002: ("extension", "Extension loaded: {} (version={}, capabilities={})"),
    3003: ("extension", "Extension load failed: {} (path={}, error={})"),
    3010: ("extension", "Extension initialized: {}"),
    3011: ("extension", "Extension init failed: {} (rc={})"),
    3012: ("extension", "Extension uninitialized: {}"),
    3020: ("extension", "Scanning extension directory: {}"),
    3021: ("extension", "Extension scan complete: {} ({} extensions found)"),
    3030: ("extension", "Capability matched: {} -> extension {}"),
    3031: ("extension", "No extension found for capability: {}"),
    # Communication (4xxx)
    4001: ("comm", "Connected to service endpoint: {} (protocol={})"),
    4002: ("comm", "Connection failed: {} (rc={}, attempt={}/{})"),
    4003: ("comm", "Disconnected from service: {} (reason={})"),
    4010: ("comm", "Polling for deployment (endpoint={})"),
    4011: ("comm", "Poll result: {} (status={}, latency={} ms)"),
    4020: ("comm", "Reporting device state: {} (workflow={})"),
    4021: ("comm", "State report accepted (workflow={}, code={})"),
    4022: ("comm", "State report failed (workflow={}, rc={}, retry={})"),
    # Download (5xxx)
    5001: ("download", "Download started: {} (size={} bytes, dest={})"),
    5002: ("download", "Download progress: {} ({}/{} bytes, {}%)"),
    5003: ("download", "Download complete: {} (size={} bytes, duration={} ms)"),
    5004: ("download", "Download failed: {} (rc={}, http_status={})"),
    5010: ("download", "Download retry: {} (attempt={}/{}, backoff={} ms)"),
    5020: ("download", "Hash verification passed: {} (algorithm={})"),
    5021: ("download", "Hash mismatch: {} (expected={}, actual={})"),
    # Security (6xxx)
    6001: ("security", "Certificate loaded: {} (subject={}, expires={})"),
    6002: ("security", "Certificate expired: {} (expired={})"),
    6010: ("security", "Signature verification passed: {} (signer={})"),
    6011: ("security", "Signature verification failed: {} (error={})"),
    6020: ("security", "Signing key loaded: {} (type={}, bits={})"),
    6021: ("security", "Signing key load failed: {} (error={})"),
    # DAG Engine (7xxx)
    7001: ("dag", "DAG created: {} ({} nodes, {} edges)"),
    7002: ("dag", "Cycle detected in DAG: {} (nodes involved: {})"),
    7010: ("dag", "Node ready for execution: {} (dependencies satisfied)"),
    7011: ("dag", "Node completed: {} (duration={} ms)"),
    7012: ("dag", "Node failed: {} (rc={})"),
    7013: ("dag", "Node skipped: {} (reason={})"),
    7020: ("dag", "DAG execution complete: {} (nodes_ok={}, nodes_failed={})"),
    7030: ("dag", "Dependency resolved: {} -> {}"),
    7031: ("dag", "Skipping node {} due to failed dependency: {}"),
    7032: ("dag", "Running node {} despite failed dependency (runOnFailed=true)"),
    # Script Handler (8xxx)
    8001: ("script", "Evaluating script handler for step: {} (type={})"),
    8010: ("script", "Script execution started: {} (cmd={}, args={})"),
    8011: ("script", "Script completed: {} (exit_code={}, duration={} ms)"),
    8012: ("script", "Script timed out: {} (timeout={} seconds, pid={})"),
    8013: ("script", "Script failed: {} (exit_code={}, stderr={})"),
    8020: ("script", "Script rollback started: {} (original_step={})"),
    8030: ("script", "Script installed check: {} (result={})"),
    8040: ("script", "Script result file: {} (path={}, rc={})"),
    # APT Handler (9xxx)
    9001: ("apt", "Evaluating APT handler for step: {} (packages={})"),
    9010: ("apt", "Updating APT package index (sources={})"),
    9020: ("apt", "APT install started: {} (packages={})"),
    9021: ("apt", "APT install complete: {} ({} packages installed)"),
    9022: ("apt", "APT install failed: {} (package={}, error={})"),
    9030: ("apt", "APT validation: {} (package={}, expected={}, actual={})"),
    9040: ("apt", "APT rollback started: {} (packages={})"),
    # SWUpdate Handler (0xAxxx)
    0xA001: ("swupdate", "Evaluating SWUpdate handler for step: {}"),
    0xA010: ("swupdate", "SWUpdate execution started: {} (image={})"),
    0xA011: ("swupdate", "SWUpdate progress: {} ({}% complete)"),
    0xA012: ("swupdate", "SWUpdate complete: {} (duration={} ms)"),
    0xA013: ("swupdate", "SWUpdate failed: {} (rc={}, detail={})"),
    0xA020: ("swupdate", "SWUpdate verification: {} (slot={}, status={})"),
    0xA030: ("swupdate", "SWUpdate reboot signal sent (delay={} seconds)"),
}

# ─── Known result codes for inline decoding ──────────────────────────────────

RESULT_CODES = {
    0x00000000: "SUCCESS",
    0x80040001: "E_NOTIMPL",
    0x80040002: "E_OUTOFMEMORY",
    0x80040003: "E_INVALIDARG",
    0x80040004: "E_NOINTERFACE",
    0x80048000: "ADUC_E_GENERAL_FAILURE",
    0x80048001: "ADUC_E_DOWNLOAD_FAILURE",
    0x80048002: "ADUC_E_INSTALL_FAILURE",
    0x80048003: "ADUC_E_APPLY_FAILURE",
    0x80048004: "ADUC_E_CANCEL_FAILURE",
    0x80048005: "ADUC_E_WORKFLOW_CANCELLED",
    0x80048006: "ADUC_E_HANDLER_NOT_FOUND",
    0x80048007: "ADUC_E_OPERATION_TIMEOUT",
    0x80048008: "ADUC_E_HASH_VERIFICATION_FAILED",
    0x80048009: "ADUC_E_SIGNATURE_VERIFICATION_FAILED",
    0x8004800A: "ADUC_E_CERT_EXPIRED",
    0x8004800B: "ADUC_E_EXTENSION_LOAD_FAILED",
    0x8004800C: "ADUC_E_CONNECTION_FAILED",
    0x8004800D: "ADUC_E_CONFIG_ERROR",
    0x8004800E: "ADUC_E_DAG_CYCLE_DETECTED",
}

# ─── Log levels ──────────────────────────────────────────────────────────────

LEVELS = ["FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"]
LEVEL_VALUES = {name: idx for idx, name in enumerate(LEVELS)}
LEVEL_BINARY = {0: "FATAL", 1: "ERROR", 2: "WARN", 3: "INFO", 4: "DEBUG", 5: "TRACE"}

# ─── ANSI color codes ────────────────────────────────────────────────────────

COLORS = {
    "FATAL": "\033[1;31m",
    "ERROR": "\033[31m",
    "WARN": "\033[33m",
    "INFO": "\033[32m",
    "DEBUG": "\033[36m",
    "TRACE": "\033[90m",
    "RESET": "\033[0m",
    "BOLD": "\033[1m",
    "DIM": "\033[2m",
}


def supports_color():
    """Check if the terminal supports ANSI color codes."""
    if os.environ.get("NO_COLOR"):
        return False
    if sys.platform == "win32":
        return os.environ.get("TERM") == "xterm" or "WT_SESSION" in os.environ
    return hasattr(sys.stdout, "isatty") and sys.stdout.isatty()


# ─── Log entry data class ────────────────────────────────────────────────────


class LogEntry:
    """Represents a single parsed log entry."""

    __slots__ = ("timestamp", "level", "component", "event_id", "message", "raw_line")

    def __init__(self, timestamp, level, component, event_id, message, raw_line=""):
        self.timestamp = timestamp
        self.level = level
        self.component = component
        self.event_id = event_id
        self.message = message
        self.raw_line = raw_line

    def to_dict(self):
        """Convert to JSON-serializable dictionary."""
        return {
            "timestamp": self.timestamp.isoformat() if self.timestamp else None,
            "level": self.level,
            "component": self.component,
            "event_id": self.event_id,
            "message": self.message,
        }


# ─── Text log parser ─────────────────────────────────────────────────────────

# Pattern: 2024-01-15T10:30:00.123Z [INFO] [workflow] (2001) Message text
TEXT_LOG_PATTERN = re.compile(
    r"^(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(?:\.\d+)?Z?)\s+"
    r"\[(\w+)\]\s+"
    r"\[(\w+)\]\s+"
    r"\((\d+)\)\s+"
    r"(.*)$"
)


def parse_text_line(line):
    """Parse a single text-format log line. Returns LogEntry or None."""
    line = line.rstrip("\n\r")
    m = TEXT_LOG_PATTERN.match(line)
    if not m:
        return None

    ts_str, level, component, event_id_str, message = m.groups()

    try:
        # Handle timestamps with and without fractional seconds
        if "." in ts_str:
            ts = datetime.strptime(ts_str.rstrip("Z"), "%Y-%m-%dT%H:%M:%S.%f")
        else:
            ts = datetime.strptime(ts_str.rstrip("Z"), "%Y-%m-%dT%H:%M:%S")
        ts = ts.replace(tzinfo=timezone.utc)
    except ValueError:
        ts = None

    return LogEntry(
        timestamp=ts,
        level=level.upper(),
        component=component,
        event_id=int(event_id_str),
        message=message,
        raw_line=line,
    )


# ─── Binary log parser ───────────────────────────────────────────────────────

# Binary format:
# [timestamp:8 bytes, uint64 epoch_ms][level:1 byte][component_len:1 byte]
# [component:N bytes][event_id:2 bytes, uint16][msg_len:2 bytes, uint16][message:N bytes]

BINARY_HEADER_SIZE = 8 + 1 + 1  # timestamp + level + component_len


def parse_binary_entry(f):
    """Parse a single binary log entry from file object. Returns LogEntry or None."""
    header = f.read(BINARY_HEADER_SIZE)
    if len(header) < BINARY_HEADER_SIZE:
        return None

    try:
        epoch_ms, level_byte, comp_len = struct.unpack("<QBB", header)
    except struct.error:
        return None

    comp_data = f.read(comp_len)
    if len(comp_data) < comp_len:
        return None

    evt_msg_hdr = f.read(4)  # event_id(2) + msg_len(2)
    if len(evt_msg_hdr) < 4:
        return None

    try:
        event_id, msg_len = struct.unpack("<HH", evt_msg_hdr)
    except struct.error:
        return None

    msg_data = f.read(msg_len)
    if len(msg_data) < msg_len:
        return None

    try:
        ts = datetime.fromtimestamp(epoch_ms / 1000.0, tz=timezone.utc)
    except (OSError, ValueError):
        ts = None

    level = LEVEL_BINARY.get(level_byte, "INFO")
    component = comp_data.decode("utf-8", errors="replace")
    message = msg_data.decode("utf-8", errors="replace")

    # If message is empty, try to use the event table format string
    if not message and event_id in EVENT_TABLE:
        _, fmt = EVENT_TABLE[event_id]
        message = fmt

    return LogEntry(
        timestamp=ts, level=level, component=component, event_id=event_id, message=message
    )


def is_binary_log(filepath):
    """Detect if a file is in binary log format by checking first bytes."""
    try:
        with open(filepath, "rb") as f:
            header = f.read(BINARY_HEADER_SIZE)
            if len(header) < BINARY_HEADER_SIZE:
                return False
            _, level_byte, comp_len = struct.unpack("<QBB", header)
            # Heuristic: level should be 0-5, component_len should be reasonable
            return level_byte <= 5 and 1 <= comp_len <= 64
    except (OSError, struct.error):
        return False


# ─── Log file reader ─────────────────────────────────────────────────────────


def read_text_log(filepath):
    """Read all entries from a text log file."""
    entries = []
    try:
        with open(filepath, "r", encoding="utf-8", errors="replace") as f:
            for line in f:
                entry = parse_text_line(line)
                if entry:
                    entries.append(entry)
    except OSError as e:
        print(f"Error reading file: {e}", file=sys.stderr)
    return entries


def read_binary_log(filepath):
    """Read all entries from a binary log file."""
    entries = []
    try:
        with open(filepath, "rb") as f:
            while True:
                entry = parse_binary_entry(f)
                if entry is None:
                    break
                entries.append(entry)
    except OSError as e:
        print(f"Error reading file: {e}", file=sys.stderr)
    return entries


def read_log(filepath, fmt=None):
    """Read log file, auto-detecting format if not specified."""
    if fmt == "binary":
        return read_binary_log(filepath)
    elif fmt == "text":
        return read_text_log(filepath)
    else:
        if is_binary_log(filepath):
            return read_binary_log(filepath)
        return read_text_log(filepath)


# ─── Filtering ────────────────────────────────────────────────────────────────


def filter_entries(entries, args):
    """Apply command-line filters to log entries."""
    filtered = entries

    # Filter by component
    if args.component:
        components = {c.lower() for c in args.component.split(",")}
        filtered = [e for e in filtered if e.component.lower() in components]

    # Filter by level (show this level and above)
    if args.level:
        max_level_val = LEVEL_VALUES.get(args.level.upper(), 3)
        filtered = [e for e in filtered if LEVEL_VALUES.get(e.level, 3) <= max_level_val]

    # Filter by event ID range
    if args.event_id:
        if "-" in args.event_id:
            lo, hi = args.event_id.split("-", 1)
            lo, hi = int(lo), int(hi)
            filtered = [e for e in filtered if lo <= e.event_id <= hi]
        else:
            target = int(args.event_id)
            filtered = [e for e in filtered if e.event_id == target]

    # Filter by time range
    if args.since:
        since_dt = _parse_time(args.since)
        if since_dt:
            filtered = [e for e in filtered if e.timestamp and e.timestamp >= since_dt]

    if args.until:
        until_dt = _parse_time(args.until)
        if until_dt:
            filtered = [e for e in filtered if e.timestamp and e.timestamp <= until_dt]

    # Filter by keyword/grep
    if args.grep:
        pattern = re.compile(args.grep, re.IGNORECASE)
        filtered = [e for e in filtered if pattern.search(e.message)]

    return filtered


def _parse_time(time_str):
    """Parse a time string into a datetime object."""
    formats = [
        "%Y-%m-%dT%H:%M:%S.%fZ",
        "%Y-%m-%dT%H:%M:%SZ",
        "%Y-%m-%dT%H:%M:%S.%f",
        "%Y-%m-%dT%H:%M:%S",
        "%Y-%m-%d %H:%M:%S.%f",
        "%Y-%m-%d %H:%M:%S",
        "%Y-%m-%d",
    ]
    for fmt in formats:
        try:
            dt = datetime.strptime(time_str, fmt)
            return dt.replace(tzinfo=timezone.utc)
        except ValueError:
            continue
    return None


# ─── Result code decoding ────────────────────────────────────────────────────

RC_PATTERN = re.compile(r"(rc=|0x)([0-9a-fA-F]{8})")


def decode_result_codes(message):
    """Replace hex result codes in a message with their symbolic names."""

    def replace_rc(m):
        prefix = m.group(1)
        hex_str = m.group(2)
        val = int(hex_str, 16)
        name = RESULT_CODES.get(val)
        if name:
            return f"{prefix}{hex_str} [{name}]"
        return m.group(0)

    return RC_PATTERN.sub(replace_rc, message)


# ─── Output formatting ───────────────────────────────────────────────────────


def format_entry(entry, color=True, decode_rc=False):
    """Format a log entry for terminal display."""
    ts_str = entry.timestamp.strftime("%Y-%m-%d %H:%M:%S.%f")[:-3] if entry.timestamp else "????-??-?? ??:??:??.???"
    msg = entry.message
    if decode_rc:
        msg = decode_result_codes(msg)

    if color and supports_color():
        level_color = COLORS.get(entry.level, "")
        reset = COLORS["RESET"]
        dim = COLORS["DIM"]
        return (
            f"{dim}{ts_str}{reset} "
            f"{level_color}[{entry.level:<5}]{reset} "
            f"{dim}[{entry.component}]{reset} "
            f"{dim}({entry.event_id}){reset} "
            f"{msg}"
        )
    else:
        return f"{ts_str} [{entry.level:<5}] [{entry.component}] ({entry.event_id}) {msg}"


def output_json(entries, decode_rc=False):
    """Output entries as JSON array."""
    result = []
    for entry in entries:
        d = entry.to_dict()
        if decode_rc:
            d["message"] = decode_result_codes(d["message"])
        result.append(d)
    print(json.dumps(result, indent=2, default=str))


# ─── Statistics mode ─────────────────────────────────────────────────────────


def show_statistics(entries):
    """Display statistical analysis of log entries."""
    if not entries:
        print("No log entries to analyze.")
        return

    total = len(entries)

    # Time span
    timestamps = [e.timestamp for e in entries if e.timestamp]
    if timestamps:
        start_time = min(timestamps)
        end_time = max(timestamps)
        duration = end_time - start_time
    else:
        start_time = end_time = None
        duration = timedelta(0)

    # Counts
    level_counts = Counter(e.level for e in entries)
    component_counts = Counter(e.component for e in entries)
    error_events = Counter(
        e.event_id for e in entries if e.level in ("ERROR", "FATAL")
    )

    # Output
    print()
    print("Log Analysis Report")
    print("\u2550" * 50)
    print(f"Total entries:     {total:,}")

    if start_time and end_time:
        dur_str = _format_duration(duration)
        print(
            f"Time span:         {start_time.strftime('%Y-%m-%d %H:%M:%S')} "
            f"\u2192 {end_time.strftime('%H:%M:%S')} ({dur_str})"
        )
    print()

    # By Level
    print("By Level:")
    for level in LEVELS:
        count = level_counts.get(level, 0)
        if count > 0:
            pct = 100.0 * count / total
            bar_len = int(40 * count / total)
            bar = "\u2588" * bar_len
            print(f"  {level:<8} {count:>5} ({pct:>5.1f}%)  {bar}")
    print()

    # By Component
    print("By Component:")
    for comp, count in component_counts.most_common(10):
        pct = 100.0 * count / total
        print(f"  {comp:<12} {count:>5} ({pct:.1f}%)")
    print()

    # Top Errors
    if error_events:
        print("Top Errors:")
        for event_id, count in error_events.most_common(10):
            name = _event_name(event_id)
            print(f"  {event_id}: {name} ({count} occurrence{'s' if count != 1 else ''})")
        print()


def _format_duration(td):
    """Format a timedelta as a human-readable string."""
    total_seconds = int(td.total_seconds())
    if total_seconds < 60:
        return f"{total_seconds}s"
    elif total_seconds < 3600:
        m, s = divmod(total_seconds, 60)
        return f"{m}m {s}s"
    else:
        h, remainder = divmod(total_seconds, 3600)
        m, s = divmod(remainder, 60)
        return f"{h}h {m}m {s}s"


def _event_name(event_id):
    """Get a human-readable name for an event ID."""
    if event_id in EVENT_TABLE:
        component, _ = EVENT_TABLE[event_id]
        return f"{component.upper()}_{event_id}"
    return f"UNKNOWN_{event_id}"


# ─── Follow mode ─────────────────────────────────────────────────────────────


def follow_log(filepath, args):
    """Follow a text log file (tail -f equivalent)."""
    color = not args.no_color
    try:
        with open(filepath, "r", encoding="utf-8", errors="replace") as f:
            # Seek to end
            f.seek(0, 2)
            print(f"Following {filepath} (Ctrl+C to stop)...", file=sys.stderr)
            while True:
                line = f.readline()
                if line:
                    entry = parse_text_line(line)
                    if entry:
                        if _entry_matches_filter(entry, args):
                            print(format_entry(entry, color=color, decode_rc=args.decode_rc))
                else:
                    time.sleep(0.1)
    except KeyboardInterrupt:
        print("\nStopped.", file=sys.stderr)
    except OSError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


def _entry_matches_filter(entry, args):
    """Quick filter check for follow mode."""
    if args.component:
        components = {c.lower() for c in args.component.split(",")}
        if entry.component.lower() not in components:
            return False
    if args.level:
        max_level_val = LEVEL_VALUES.get(args.level.upper(), 3)
        if LEVEL_VALUES.get(entry.level, 3) > max_level_val:
            return False
    if args.grep:
        if not re.search(args.grep, entry.message, re.IGNORECASE):
            return False
    return True


# ─── Grep with context ───────────────────────────────────────────────────────


def grep_with_context(entries, pattern, context_lines, color=True, decode_rc=False):
    """Show matching entries with surrounding context lines."""
    matches = set()
    pat = re.compile(pattern, re.IGNORECASE)

    for i, entry in enumerate(entries):
        if pat.search(entry.message):
            for j in range(max(0, i - context_lines), min(len(entries), i + context_lines + 1)):
                matches.add(j)

    last_printed = -2
    for i in sorted(matches):
        if i > last_printed + 1:
            print("---")
        entry = entries[i]
        line = format_entry(entry, color=color, decode_rc=decode_rc)
        # Highlight matching text
        if pat.search(entry.message) and color and supports_color():
            line = f"\033[1m{line}\033[0m"
        print(line)
        last_printed = i


# ─── Main ────────────────────────────────────────────────────────────────────


def build_parser():
    """Build the argument parser."""
    parser = argparse.ArgumentParser(
        prog="log_viewer",
        description="ADU Gen2 Agent Log Viewer/Processor",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""\
Examples:
  %(prog)s /var/log/adu-gen2.log
  %(prog)s --component workflow --level error log.txt
  %(prog)s --since "2024-01-15 10:00:00" --until "2024-01-15 11:00:00" log.bin
  %(prog)s --stats /var/log/adu-gen2.log
  %(prog)s --follow /var/log/adu-gen2.log
  %(prog)s --decode-rc /var/log/adu-gen2.log
  %(prog)s --json /var/log/adu-gen2.log
  %(prog)s --grep "failed" --context 3 /var/log/adu-gen2.log
""",
    )

    parser.add_argument("logfile", help="Path to log file")
    parser.add_argument(
        "--format",
        choices=["text", "binary", "auto"],
        default="auto",
        help="Log file format (default: auto-detect)",
    )

    # Filtering
    filter_group = parser.add_argument_group("filtering")
    filter_group.add_argument(
        "--component", "-c", help="Filter by component (comma-separated, e.g. workflow,comm)"
    )
    filter_group.add_argument(
        "--level",
        "-l",
        choices=["fatal", "error", "warn", "info", "debug", "trace"],
        help="Minimum log level to display",
    )
    filter_group.add_argument(
        "--event-id", "-e", help="Filter by event ID or range (e.g. 2001 or 2001-2020)"
    )
    filter_group.add_argument("--since", help="Show entries after this time")
    filter_group.add_argument("--until", help="Show entries before this time")
    filter_group.add_argument("--grep", "-g", help="Filter by regex pattern in message")
    filter_group.add_argument(
        "--context", type=int, default=0, help="Lines of context around grep matches"
    )

    # Output modes
    output_group = parser.add_argument_group("output")
    output_group.add_argument("--json", action="store_true", help="Output as JSON")
    output_group.add_argument("--stats", action="store_true", help="Show log statistics")
    output_group.add_argument("--follow", "-f", action="store_true", help="Follow log file (tail -f)")
    output_group.add_argument(
        "--decode-rc", action="store_true", help="Decode result codes inline"
    )
    output_group.add_argument("--no-color", action="store_true", help="Disable colorized output")
    output_group.add_argument(
        "--count", action="store_true", help="Only print the count of matching entries"
    )

    return parser


def main():
    """Main entry point."""
    parser = build_parser()
    args = parser.parse_args()

    # Validate file exists
    if not os.path.exists(args.logfile):
        print(f"Error: File not found: {args.logfile}", file=sys.stderr)
        sys.exit(1)

    # Follow mode
    if args.follow:
        follow_log(args.logfile, args)
        return

    # Read log file
    fmt = None if args.format == "auto" else args.format
    entries = read_log(args.logfile, fmt=fmt)

    if not entries:
        print("No log entries found.", file=sys.stderr)
        sys.exit(0)

    # Apply filters
    entries = filter_entries(entries, args)

    # Count mode
    if args.count:
        print(len(entries))
        return

    # Statistics mode
    if args.stats:
        show_statistics(entries)
        return

    # Grep with context
    if args.grep and args.context > 0:
        grep_with_context(
            entries,
            args.grep,
            args.context,
            color=not args.no_color,
            decode_rc=args.decode_rc,
        )
        return

    # JSON output
    if args.json:
        output_json(entries, decode_rc=args.decode_rc)
        return

    # Standard output
    color = not args.no_color
    for entry in entries:
        print(format_entry(entry, color=color, decode_rc=args.decode_rc))


if __name__ == "__main__":
    main()
