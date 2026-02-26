# extensions

**Type:** Plugin/Extension Framework (Shared Libraries + Static Libraries)

## Description

Implements the **dynamically-loadable extension architecture** for the Device Update agent. This framework allows the agent to support multiple update types, download methods, and device component enumeration strategies through a plugin-based system.

## Submodules

### Core Framework
- **`extension_manager/`** — Central `ExtensionManager` class that loads, registers, and dispatches to extension shared libraries at runtime. Manages the lifecycle of all extension types.
- **`shared_lib/`** — Shared library loading utilities used by the extension manager.
- **`inc/`** — Shared header files and interface definitions for extensions.

### Content Downloaders
- **`content_downloaders/`** — Plugins for downloading update content from the cloud:
  - **curl-based downloader** — Uses libcurl for HTTP/HTTPS downloads
  - **Delivery Optimization downloader** — Uses Microsoft Delivery Optimization for bandwidth-efficient downloads

### Step Handlers
- **`step_handlers/`** — Update-type-specific handlers that implement the `ContentHandler` interface (`Download`, `Install`, `Apply`, `Backup`, `Restore`, `Cancel`, `IsInstalled`):
  - **APT handler** — Installs/removes Debian packages via `apt-get`
  - **Script handler** — Executes custom update scripts
  - **SWUpdate v2 handler** — Integrates with the SWUpdate framework for embedded Linux updates
  - **Simulator handler** — Simulates update operations for testing
  - **WIM handler** — Windows Imaging format handler

### Update Manifest Handlers
- **`update_manifest_handlers/`** — Handlers for processing update manifests:
  - **Steps handler** — Orchestrates multi-step updates with ordered execution

### Download Handlers
- **`download_handlers/`** — Plugin system for custom download processing (e.g., delta/diff downloads). Includes a factory and plugin loader.

### Component Enumerators
- **`component_enumerators/`** — Plugins that enumerate device sub-components for component-targeted updates (e.g., enumerating firmware components on a multi-component device).

## Dependencies

Depends on `adu_types`, `c_utils`, `contract_utils`, and platform-specific dynamic library loading (`dlopen`/`dlsym`).
