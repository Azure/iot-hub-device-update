# rootkey_workflow

**Type:** Static C Library (`rootkey_workflow`)

## Description

Implements the **root key update workflow** for the Device Update agent. Root keys are used to verify the authenticity and integrity of update packages. This module handles the secure lifecycle of downloading, validating, and storing root key packages.

## Key Function

- `RootKeyWorkflow_UpdateRootKeys()` — Main entry point that orchestrates the entire root key update process.

## Workflow Steps

1. **Download** the root key package from a specified URL (supports both curl and Delivery Optimization backends)
2. **Parse** the downloaded root key package
3. **Validate** the package against hardcoded trusted keys
4. **Compare** with the existing local root key store to determine if an update is needed
5. **Atomically write** the new root key package to disk if an update is required

## Security

- Validates root key packages against hardcoded keys built into the agent
- Enforces test vs. production package separation
- Uses atomic file writes to prevent corruption during updates

## Dependencies

Depends on `root_key_utils`, `rootkeypackage_utils`, `crypto_utils`, `c_utils`, and download backends (curl/Delivery Optimization).
