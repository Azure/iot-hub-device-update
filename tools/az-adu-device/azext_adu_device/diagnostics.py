"""Diagnostic log bundle collection for the ADU agent."""

import datetime
import io
import json
import os
import platform
import re
import shutil
import subprocess
import tarfile
import tempfile


# Secrets patterns to redact from config files
_SECRET_PATTERNS = [
    re.compile(r'(connection_string\s*=\s*")[^"]*(")', re.IGNORECASE),
    re.compile(r'(shared_access_key\s*=\s*")[^"]*(")', re.IGNORECASE),
    re.compile(r'(sas_token\s*=\s*")[^"]*(")', re.IGNORECASE),
    re.compile(r'(password\s*=\s*")[^"]*(")', re.IGNORECASE),
    re.compile(r'(secret\s*=\s*")[^"]*(")', re.IGNORECASE),
]


def _sanitize_config(content):
    """Redact secrets from configuration file content."""
    for pattern in _SECRET_PATTERNS:
        content = pattern.sub(r"\1***REDACTED***\2", content)
    return content


class DiagnosticCollector:
    """Collects diagnostic information from the ADU agent and device."""

    DEFAULT_LOG_DIR = "/var/log/adu"
    DEFAULT_CONFIG_PATH = "/etc/adu/agent.toml"
    DEFAULT_CORE_DUMP_DIRS = ["/var/crash", "/var/lib/systemd/coredump"]
    ADU_SERVICE_NAME = "adu-agent"

    def __init__(self, log_dir=None, config_path=None):
        self.log_dir = log_dir or self.DEFAULT_LOG_DIR
        self.config_path = config_path or self.DEFAULT_CONFIG_PATH

    def collect_logs(self, since=None):
        """Collect ADU agent log files.

        Returns a list of (archive_name, file_path) tuples.
        """
        files = []
        if not os.path.isdir(self.log_dir):
            return files

        since_dt = self._parse_since(since) if since else None

        for name in sorted(os.listdir(self.log_dir)):
            path = os.path.join(self.log_dir, name)
            if not os.path.isfile(path):
                continue
            if since_dt:
                mtime = datetime.datetime.fromtimestamp(os.path.getmtime(path))
                if mtime < since_dt:
                    continue
            files.append((f"logs/{name}", path))
        return files

    def collect_journal(self, since=None):
        """Collect systemd journal entries for the ADU agent service.

        Returns journal text content or None if unavailable.
        """
        cmd = [
            "journalctl",
            "-u", self.ADU_SERVICE_NAME,
            "--no-pager",
            "-o", "short-iso",
        ]
        if since:
            cmd.extend(["--since", self._normalize_since_for_journalctl(since)])

        try:
            result = subprocess.run(
                cmd, capture_output=True, text=True, timeout=30
            )
            if result.returncode == 0 and result.stdout.strip():
                return result.stdout
        except (FileNotFoundError, subprocess.TimeoutExpired):
            pass
        return None

    def collect_config(self, sanitize=True):
        """Read agent config file, optionally sanitizing secrets.

        Returns (content_string, config_path) or (None, None).
        """
        if not os.path.isfile(self.config_path):
            return None, None

        try:
            with open(self.config_path, "r", encoding="utf-8") as f:
                content = f.read()
        except PermissionError:
            return None, None

        if sanitize:
            content = _sanitize_config(content)
        return content, self.config_path

    def collect_system_info(self):
        """Collect basic system information.

        Returns a dict with uname, disk, memory info.
        """
        info = {
            "timestamp": datetime.datetime.utcnow().isoformat() + "Z",
            "uname": {
                "system": platform.system(),
                "node": platform.node(),
                "release": platform.release(),
                "version": platform.version(),
                "machine": platform.machine(),
            },
        }

        # Disk usage for key paths
        for mount in ["/", "/var/log/adu", "/etc/adu"]:
            try:
                usage = shutil.disk_usage(mount)
                info.setdefault("disk", {})[mount] = {
                    "total_mb": round(usage.total / (1024 * 1024)),
                    "free_mb": round(usage.free / (1024 * 1024)),
                    "used_percent": round(
                        (usage.used / usage.total) * 100, 1
                    ),
                }
            except OSError:
                pass

        # Memory info from /proc/meminfo
        try:
            with open("/proc/meminfo", "r") as f:
                meminfo = {}
                for line in f:
                    parts = line.split(":")
                    if len(parts) == 2:
                        key = parts[0].strip()
                        val = parts[1].strip()
                        if key in ("MemTotal", "MemFree", "MemAvailable", "SwapTotal", "SwapFree"):
                            meminfo[key] = val
                if meminfo:
                    info["memory"] = meminfo
        except OSError:
            pass

        return info

    def collect_core_dumps(self):
        """Find core dump files related to ADU.

        Returns list of (archive_name, file_path) tuples.
        """
        files = []
        for dump_dir in self.DEFAULT_CORE_DUMP_DIRS:
            if not os.path.isdir(dump_dir):
                continue
            for name in os.listdir(dump_dir):
                if "adu" in name.lower():
                    path = os.path.join(dump_dir, name)
                    if os.path.isfile(path):
                        files.append((f"core_dumps/{name}", path))
        return files

    def collect_agent_status(self, client):
        """Get a status snapshot from the running agent.

        Returns status dict or None on failure.
        """
        try:
            return client.get_status()
        except Exception:
            return None

    def create_bundle(
        self, output_path, client=None, since=None, include_core_dumps=False
    ):
        """Create a .tar.gz diagnostic bundle.

        Args:
            output_path: Where to write the bundle file.
            client: Optional ADUAgentClient for live agent queries.
            since: Time filter for logs.
            include_core_dumps: Whether to include core dump files.

        Returns:
            Path to the created bundle and a manifest dict.
        """
        manifest = {
            "created": datetime.datetime.utcnow().isoformat() + "Z",
            "components": [],
        }

        with tarfile.open(output_path, "w:gz") as tar:
            # Agent log files
            log_files = self.collect_logs(since=since)
            for arc_name, file_path in log_files:
                tar.add(file_path, arcname=arc_name)
            if log_files:
                manifest["components"].append(
                    {"type": "agent_logs", "count": len(log_files)}
                )

            # Journal entries
            journal = self.collect_journal(since=since)
            if journal:
                self._add_string_to_tar(tar, "journal/adu-agent.log", journal)
                manifest["components"].append({"type": "journal"})

            # Config (sanitized)
            config_content, _ = self.collect_config(sanitize=True)
            if config_content:
                self._add_string_to_tar(tar, "config/agent.toml", config_content)
                manifest["components"].append({"type": "config"})

            # System info
            sys_info = self.collect_system_info()
            self._add_string_to_tar(
                tar,
                "system/info.json",
                json.dumps(sys_info, indent=2),
            )
            manifest["components"].append({"type": "system_info"})

            # Agent status snapshot
            if client:
                status = self.collect_agent_status(client)
                if status:
                    self._add_string_to_tar(
                        tar,
                        "agent/status.json",
                        json.dumps(status, indent=2),
                    )
                    manifest["components"].append({"type": "agent_status"})

            # Core dumps
            if include_core_dumps:
                dumps = self.collect_core_dumps()
                for arc_name, file_path in dumps:
                    tar.add(file_path, arcname=arc_name)
                if dumps:
                    manifest["components"].append(
                        {"type": "core_dumps", "count": len(dumps)}
                    )

            # Write manifest
            self._add_string_to_tar(
                tar, "manifest.json", json.dumps(manifest, indent=2)
            )

        return output_path, manifest

    def collect_remote(self, ssh_host, ssh_user, ssh_key=None, since=None,
                       include_core_dumps=False, output_path=None):
        """Collect diagnostics from a remote device via SSH.

        Uses paramiko to SSH into the device, run the diagnostic collection
        remotely, and download the resulting bundle.

        Returns:
            Local path to the downloaded bundle.
        """
        import paramiko

        ssh = paramiko.SSHClient()
        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())

        connect_kwargs = {
            "hostname": ssh_host,
            "username": ssh_user,
        }
        if ssh_key:
            connect_kwargs["key_filename"] = ssh_key

        ssh.connect(**connect_kwargs)

        try:
            # Build remote command
            remote_bundle = "/tmp/adu_diag_bundle.tar.gz"
            cmd_parts = [
                "python3", "-m", "azext_adu_device.diagnostics_runner",
                "--output", remote_bundle,
            ]
            if since:
                cmd_parts.extend(["--since", since])
            if include_core_dumps:
                cmd_parts.append("--include-core-dumps")

            # If the module isn't available, fall back to a tar-based collection
            fallback_cmd = (
                f"tar czf {remote_bundle} "
                f"/var/log/adu/ /etc/adu/agent.toml 2>/dev/null; "
                f"echo DONE"
            )
            cmd = (
                f"({' '.join(cmd_parts)} 2>/dev/null && echo DONE) || "
                f"({fallback_cmd})"
            )

            _, stdout, stderr = ssh.exec_command(cmd, timeout=120)
            exit_status = stdout.channel.recv_exit_status()

            if exit_status != 0:
                err = stderr.read().decode(errors="replace")
                raise RuntimeError(
                    f"Remote diagnostic collection failed (exit {exit_status}): {err}"
                )

            # Download bundle
            if not output_path:
                output_path = os.path.join(
                    os.getcwd(),
                    f"adu_diag_{ssh_host}_{datetime.datetime.now():%Y%m%d_%H%M%S}.tar.gz",
                )

            sftp = ssh.open_sftp()
            try:
                sftp.get(remote_bundle, output_path)
            finally:
                # Clean up remote temp file
                try:
                    sftp.remove(remote_bundle)
                except OSError:
                    pass
                sftp.close()

            return output_path
        finally:
            ssh.close()

    @staticmethod
    def _add_string_to_tar(tar, arcname, content):
        """Add a string as a file inside a tar archive."""
        data = content.encode("utf-8")
        info = tarfile.TarInfo(name=arcname)
        info.size = len(data)
        info.mtime = int(datetime.datetime.utcnow().timestamp())
        tar.addfile(info, io.BytesIO(data))

    @staticmethod
    def _parse_since(since_str):
        """Parse a relative or absolute time string into a datetime."""
        if not since_str:
            return None

        # Relative: "1h", "2d", "30m"
        match = re.match(r"^(\d+)([mhd])$", since_str)
        if match:
            value = int(match.group(1))
            unit = match.group(2)
            delta = {
                "m": datetime.timedelta(minutes=value),
                "h": datetime.timedelta(hours=value),
                "d": datetime.timedelta(days=value),
            }[unit]
            return datetime.datetime.now() - delta

        # Absolute ISO date/datetime
        for fmt in ("%Y-%m-%d", "%Y-%m-%dT%H:%M:%S", "%Y-%m-%dT%H:%M:%S%z"):
            try:
                return datetime.datetime.strptime(since_str, fmt)
            except ValueError:
                continue

        raise ValueError(f"Cannot parse time value: {since_str}")

    @staticmethod
    def _normalize_since_for_journalctl(since_str):
        """Convert a since string to journalctl --since format."""
        match = re.match(r"^(\d+)([mhd])$", since_str)
        if match:
            value = int(match.group(1))
            unit = {"m": "minutes", "h": "hours", "d": "days"}[match.group(2)]
            return f"{value} {unit} ago"
        return since_str
