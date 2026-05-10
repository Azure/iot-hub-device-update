# az-adu-device — Azure CLI Extension for ADU Agent Management

Device-side diagnostic and management tool for the Azure Device Update (ADU) Gen2 agent.

## Overview

This Azure CLI extension communicates with the ADU agent running on a device via its
Local API (Unix domain socket at `/run/adu/adu.sock`). It supports both local execution
on the device and remote access via SSH.

## Installation

```bash
az extension add --source az_adu_device-0.1.0-py3-none-any.whl
```

Or from source:

```bash
cd tools/az-adu-device
pip install -e .
```

## Commands

| Command | Description |
|---|---|
| `az adu device status` | Get agent status, installed update, connection info |
| `az adu device health` | Health check of agent and extensions |
| `az adu device scan` | Trigger an update check |
| `az adu device cancel` | Cancel an in-progress update |
| `az adu device logs collect` | Collect diagnostic log bundle (.tar.gz) |
| `az adu device logs upload` | Upload log bundle to Azure Blob Storage |
| `az adu device config show` | Display current agent configuration |
| `az adu device extensions list` | List loaded extensions |
| `az adu device update history` | Show update history |

## Wire Protocol

The agent Local API uses a binary wire protocol over Unix domain socket:

```
Request:  [ver:u16][type:u16][len:u16][payload:var]
Response: [ver:u16][status:u16][len:u16][payload:var]
```

## Examples

```bash
# Check agent status
az adu device status

# Force an update check and wait for completion
az adu device scan --wait --timeout 120

# Collect diagnostic logs from a remote device
az adu device logs collect --ssh-host 10.0.0.5 --ssh-user adu-admin --ssh-key ~/.ssh/id_rsa

# Upload a log bundle to blob storage
az adu device logs upload --bundle-path ./diag_bundle.tar.gz \
    --storage-account myaccount --container logs --sas-token "sv=2021..."

# Show agent config filtered to a section
az adu device config show --section connection
```
