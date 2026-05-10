"""Command implementations for adu device extension."""

import datetime
import json
import os
import re

from knack.log import get_logger
from knack.util import CLIError

from azext_adu_device.agent_client import ADUAgentClient, ADUAgentError
from azext_adu_device.diagnostics import DiagnosticCollector

logger = get_logger(__name__)


def _make_client(socket_path):
    """Create an ADUAgentClient, translating errors to CLIError."""
    return ADUAgentClient(socket_path=socket_path)


def _call_agent(client_method, *args, **kwargs):
    """Call an agent client method, wrapping errors as CLIError."""
    try:
        return client_method(*args, **kwargs)
    except ADUAgentError as e:
        raise CLIError(str(e))


# ---------------------------------------------------------------------------
# az adu device status
# ---------------------------------------------------------------------------
def get_status(socket_path="/run/adu/adu.sock"):
    """Get the current status of the ADU agent."""
    client = _make_client(socket_path)
    return _call_agent(client.get_status)


# ---------------------------------------------------------------------------
# az adu device health
# ---------------------------------------------------------------------------
def get_health(socket_path="/run/adu/adu.sock"):
    """Run a health check on the ADU agent."""
    client = _make_client(socket_path)
    return _call_agent(client.get_health)


# ---------------------------------------------------------------------------
# az adu device scan
# ---------------------------------------------------------------------------
def trigger_scan(socket_path="/run/adu/adu.sock", wait=False, timeout=60):
    """Trigger an update check on the ADU agent."""
    import time

    client = _make_client(socket_path)
    result = _call_agent(client.trigger_scan)

    if not wait:
        return result

    # Poll status until scan completes or times out
    deadline = time.time() + timeout
    while time.time() < deadline:
        time.sleep(2)
        try:
            status = client.get_status()
            state = status.get("state", "")
            if state.lower() not in ("scanning", "checking"):
                return status
        except ADUAgentError:
            pass

    raise CLIError(f"Scan did not complete within {timeout} seconds")


# ---------------------------------------------------------------------------
# az adu device cancel
# ---------------------------------------------------------------------------
def cancel_update(socket_path="/run/adu/adu.sock", force=False):
    """Cancel an in-progress update."""
    client = _make_client(socket_path)
    return _call_agent(client.cancel_update, force=force)


# ---------------------------------------------------------------------------
# az adu device logs collect
# ---------------------------------------------------------------------------
def logs_collect(
    socket_path="/run/adu/adu.sock",
    output_path=None,
    since=None,
    include_core_dumps=False,
    ssh_host=None,
    ssh_user=None,
    ssh_key=None,
):
    """Collect a diagnostic log bundle."""
    collector = DiagnosticCollector()

    # Remote collection via SSH
    if ssh_host:
        if not ssh_user:
            raise CLIError("--ssh-user is required when using --ssh-host")
        try:
            bundle_path = collector.collect_remote(
                ssh_host=ssh_host,
                ssh_user=ssh_user,
                ssh_key=ssh_key,
                since=since,
                include_core_dumps=include_core_dumps,
                output_path=output_path,
            )
            return {"bundlePath": bundle_path, "source": f"ssh://{ssh_user}@{ssh_host}"}
        except ImportError:
            raise CLIError(
                "paramiko is required for SSH-based collection. "
                "Install it with: pip install paramiko"
            )
        except Exception as e:
            raise CLIError(f"Remote collection failed: {e}")

    # Local collection
    if not output_path:
        timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        output_path = os.path.join(os.getcwd(), f"adu_diag_{timestamp}.tar.gz")

    try:
        client = ADUAgentClient(socket_path=socket_path)
    except Exception:
        client = None
        logger.warning("Could not connect to agent; bundle will not include live status.")

    try:
        bundle_path, manifest = collector.create_bundle(
            output_path=output_path,
            client=client,
            since=since,
            include_core_dumps=include_core_dumps,
        )
    except PermissionError:
        raise CLIError(
            "Permission denied collecting logs. Run with sudo or ensure "
            "you are in the 'adu' group."
        )
    except Exception as e:
        raise CLIError(f"Failed to create diagnostic bundle: {e}")

    return {
        "bundlePath": bundle_path,
        "manifest": manifest,
    }


# ---------------------------------------------------------------------------
# az adu device logs upload
# ---------------------------------------------------------------------------
def logs_upload(bundle_path, storage_account, container, sas_token):
    """Upload a diagnostic log bundle to Azure Blob Storage."""
    if not os.path.isfile(bundle_path):
        raise CLIError(f"Bundle file not found: {bundle_path}")

    blob_name = os.path.basename(bundle_path)
    url = (
        f"https://{storage_account}.blob.core.windows.net/"
        f"{container}/{blob_name}?{sas_token}"
    )

    try:
        # Use requests if available (azure-cli typically has it)
        import requests

        file_size = os.path.getsize(bundle_path)
        headers = {
            "x-ms-blob-type": "BlockBlob",
            "Content-Length": str(file_size),
        }

        with open(bundle_path, "rb") as f:
            response = requests.put(url, headers=headers, data=f, timeout=300)

        if response.status_code in (200, 201):
            return {
                "status": "uploaded",
                "blobUrl": (
                    f"https://{storage_account}.blob.core.windows.net/"
                    f"{container}/{blob_name}"
                ),
                "sizeBytes": file_size,
            }
        else:
            raise CLIError(
                f"Upload failed with HTTP {response.status_code}: "
                f"{response.text[:200]}"
            )
    except ImportError:
        raise CLIError("requests library is required for upload. Install with: pip install requests")
    except requests.exceptions.RequestException as e:
        raise CLIError(f"Upload failed: {e}")


# ---------------------------------------------------------------------------
# az adu device config show
# ---------------------------------------------------------------------------
def config_show(
    socket_path="/run/adu/adu.sock",
    section=None,
    config_path="/etc/adu/agent.toml",
):
    """Display current ADU agent configuration (secrets redacted)."""
    collector = DiagnosticCollector(config_path=config_path)
    content, _ = collector.collect_config(sanitize=True)

    if content is None:
        raise CLIError(f"Configuration file not found at {config_path}")

    if section:
        # Extract a TOML section (simple bracket-based parsing)
        lines = content.splitlines()
        section_lines = []
        in_section = False
        section_header = f"[{section}]"

        for line in lines:
            stripped = line.strip()
            if stripped.lower() == section_header.lower():
                in_section = True
                section_lines.append(line)
                continue
            if in_section:
                if stripped.startswith("[") and stripped != section_header:
                    break
                section_lines.append(line)

        if not section_lines:
            raise CLIError(f"Section '{section}' not found in configuration")
        content = "\n".join(section_lines)

    return {"configPath": config_path, "content": content}


# ---------------------------------------------------------------------------
# az adu device extensions list
# ---------------------------------------------------------------------------
def extensions_list(socket_path="/run/adu/adu.sock"):
    """List loaded ADU agent extensions."""
    client = _make_client(socket_path)
    health = _call_agent(client.get_health)

    extensions = health.get("extensions", [])
    if not extensions:
        logger.warning("No extensions reported by agent.")
    return extensions


# ---------------------------------------------------------------------------
# az adu device update history
# ---------------------------------------------------------------------------
def get_update_history(socket_path="/run/adu/adu.sock"):
    """Show update history from the agent's durable storage."""
    client = _make_client(socket_path)
    status = _call_agent(client.get_status)

    history = status.get("updateHistory", [])
    if not history:
        logger.warning("No update history available.")
    return history
