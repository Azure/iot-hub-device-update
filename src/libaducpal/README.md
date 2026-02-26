# libaducpal

**Type:** Static C Library (`libaducpal` — ADUC Platform Abstraction Layer)

## Description

The **Platform Abstraction Layer (PAL)** for the Device Update agent. This library provides OS-abstracted wrappers around platform-specific system calls, enabling the agent codebase to be portable across different operating systems.

## Key Abstractions

- **File system operations** — Directory creation, file permissions, path manipulation
- **Process management** — Process spawning, signal handling
- **User/group operations** — User ID lookups, permission checks
- **System information** — OS version, hostname, hardware info
- **Time operations** — Monotonic clocks, timestamps

## Purpose

By isolating platform-specific code behind a consistent API, `libaducpal` allows the rest of the agent to be written in a platform-independent manner. Platform-specific implementations are provided for each supported OS.

## Dependencies

Minimal dependencies; wraps native OS APIs (POSIX/Linux system calls).