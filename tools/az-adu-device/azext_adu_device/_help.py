"""Help text for all adu device commands."""

from knack.help_files import helps


helps["adu device"] = """
type: group
short-summary: Manage and diagnose the ADU agent running on a device.
long-summary: >
    Commands to interact with the Azure Device Update agent via its Local API
    (Unix domain socket). Supports both local on-device usage and remote access
    via SSH.
"""

helps["adu device status"] = """
type: command
short-summary: Get the current status of the ADU agent.
long-summary: >
    Connects to the agent's Local API and retrieves the current state, installed
    update ID, last check time, and connection status.
examples:
  - name: Get agent status
    text: az adu device status
  - name: Get status from a custom socket path
    text: az adu device status --socket-path /var/run/adu/agent.sock
"""

helps["adu device health"] = """
type: command
short-summary: Run a health check on the ADU agent and its extensions.
long-summary: >
    Retrieves health information including communication provider status,
    extension load status, disk space, and certificate validity.
examples:
  - name: Check agent health
    text: az adu device health
"""

helps["adu device scan"] = """
type: command
short-summary: Trigger an update check on the ADU agent.
long-summary: >
    Sends a scan trigger to the agent to check for available updates.
examples:
  - name: Trigger scan and wait for completion
    text: az adu device scan --wait --timeout 120
  - name: Trigger scan without waiting
    text: az adu device scan
"""

helps["adu device cancel"] = """
type: command
short-summary: Cancel an in-progress update.
examples:
  - name: Cancel current update
    text: az adu device cancel
  - name: Force cancel
    text: az adu device cancel --force
"""

helps["adu device logs"] = """
type: group
short-summary: Collect and upload diagnostic logs from the ADU agent.
"""

helps["adu device logs collect"] = """
type: command
short-summary: Collect a diagnostic log bundle from the device.
long-summary: >
    Gathers agent logs, system journal entries, extension logs, sanitized
    configuration, agent status snapshot, and system info into a .tar.gz bundle.
examples:
  - name: Collect logs locally
    text: az adu device logs collect --output-path ./diag_bundle.tar.gz
  - name: Collect logs from a remote device via SSH
    text: >
        az adu device logs collect --ssh-host 10.0.0.5 --ssh-user adu-admin
        --ssh-key ~/.ssh/id_rsa --output-path ./remote_diag.tar.gz
  - name: Collect logs from the last hour including core dumps
    text: az adu device logs collect --since 1h --include-core-dumps
"""

helps["adu device logs upload"] = """
type: command
short-summary: Upload a diagnostic log bundle to Azure Blob Storage.
examples:
  - name: Upload a bundle
    text: >
        az adu device logs upload --bundle-path ./diag_bundle.tar.gz
        --storage-account myaccount --container logs --sas-token "sv=2021..."
"""

helps["adu device config show"] = """
type: command
short-summary: Display the current ADU agent configuration.
long-summary: >
    Reads the agent configuration file (default /etc/adu/agent.toml) and
    displays it. Secrets are automatically redacted.
examples:
  - name: Show full config
    text: az adu device config show
  - name: Show only the connection section
    text: az adu device config show --section connection
"""

helps["adu device extensions list"] = """
type: command
short-summary: List loaded ADU agent extensions.
long-summary: >
    Connects to the agent's Local API and retrieves the extension registry,
    showing each extension's type, version, and status.
examples:
  - name: List all extensions
    text: az adu device extensions list
"""

helps["adu device update history"] = """
type: command
short-summary: Show the update history from the agent's durable storage.
long-summary: >
    Displays past update operations including workflow ID, update ID, outcome,
    and timestamp.
examples:
  - name: Show update history
    text: az adu device update history
"""
