# logging

**Type:** Static C Library (`logging`)

## Description

Provides the **centralized logging framework** for the Device Update agent. This module defines the logging API used throughout the codebase and provides a pluggable backend architecture for log output.

## Key Components

- **`inc/`** — Public logging API headers defining log macros and severity levels (e.g., `Log_Debug`, `Log_Info`, `Log_Warn`, `Log_Error`) used by all agent modules.
- **`zlog/`** — Implementation of the logging backend using the **zlog** library, a reliable, high-performance, thread-safe logging library for C. Handles log formatting, rotation, and output to files, console, or syslog.

## Features

- Multiple log severity levels
- Thread-safe logging
- Configurable log output destinations (file, console, syslog)
- Log rotation support
- Consistent log formatting across the agent

## Dependencies

Depends on the `zlog` third-party library for the logging backend implementation.
