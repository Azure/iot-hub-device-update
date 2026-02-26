# platform_layers

**Type:** Static C Libraries (per-platform implementations)

## Description

Contains **platform-specific implementations** of the Device Update agent's platform layer interface. Each subdirectory provides the concrete implementation of platform-dependent operations for a specific operating system.

## Submodules

- **`linux_platform_layer/`** — Linux-specific implementation providing:
  - Device information retrieval (manufacturer, model, OS version, etc.)
  - File system operations specific to Linux
  - System reboot and shutdown commands
  - Package management integration
  - Linux-specific path conventions and permissions

- **`windows_platform_layer/`** — Windows-specific implementation providing equivalent functionality for Windows-based devices.

## Purpose

The platform layer pattern separates OS-specific code from the core agent logic. The appropriate platform layer is selected at build time via CMake configuration, allowing the same agent codebase to target multiple operating systems.

## Dependencies

Depends on `adu_types`, `libaducpal`, and OS-specific system APIs.
