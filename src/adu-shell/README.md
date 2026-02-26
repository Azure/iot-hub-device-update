# adu-shell

**Type:** Privileged Helper Executable (`adu-shell`)

## Description

A **setuid-root helper executable** that performs operations requiring elevated permissions on behalf of the Device Update agent. The main agent process runs as a non-root user and delegates privileged operations (package installation, system reboots, script execution) to `adu-shell` via CLI invocations.

## How It Works

`adu-shell` receives commands via CLI arguments specifying:
- **Update type** — e.g., `microsoft/apt`, `microsoft/script`, or `common`
- **Action** — e.g., `download`, `install`, `apply`, `cancel`, `rollback`, `reboot`

It then dispatches to the appropriate task handler based on the update type and action.

## Supported Task Handlers

- **APT package operations** — Install/remove Debian packages via `apt-get`
- **Script execution** — Run update scripts with elevated privileges
- **Common tasks** — System reboot and other shared privileged operations

## Security

Runs as a setuid binary to bridge the privilege gap between the unprivileged agent process and operations that require root access. Only a well-defined set of operations is supported.

## Dependencies

Depends on `adu_types`, platform layer utilities, and system package management tools.
