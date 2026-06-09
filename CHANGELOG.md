## Release 1.3.1

### Other Bug Fixes

* Fix `edgegatewayCertPath` overwriting X.509 / EIS-x509 auth state — the gateway cert is now applied as a post-processing step that preserves the original authType so client cert, private key, and engine SDK options keep getting set on the IoT Hub handle (previously a non-Edge regression replaced authType with NestedEdgeCert and skipped all mTLS options)
* Fix AIS + EIS-x509 (no Edge gateway) regression where the EIS-issued identity certificate was never installed as `SU_OPTION_X509_CERT` and was mis-installed as `OPTION_TRUSTED_CERT`, causing the IoT Hub mTLS handshake to fail and the agent to restart-loop during deployments

## Release 1.3.0

### Major Features and Enhancements

* Add [X.509 client certificate authentication](docs/agent-reference/how-to-x509-authentication.md) support
* Add [CrossProc Query API (Service Status API)](docs/agent-reference/GetAduServiceStatus.md) SDK with idle pause timer and examples
* Add Microsoft Delta Download Handler support with component-based packaging
* Add Delta CacheSourceUpdate API with improved cache robustness and enhanced logging
* Use curl handler as the default content handler
* Add support for Ubuntu 24.04 LTS with GCC 13 compatibility
* Add support for Debian 13 (Trixie)
* Pass workflowData to script handler arguments for custom data propagation
* Show ExtendedResultCode in IoT Hub for "Last Attempted Update" details
* Report detailed error results for failed workflows
* Overall agent logging improvements
* Add rootkey validator tool
* Add [graceful reboot synchronization](docs/agent-reference/architecture-overview.md#graceful-reboot-flow) — lock-file protocol ensures agent completes cloud reporting and cache operations before system reboots
* Support custom work folder for dependency installation and build
* Add devcontainer with Debian 11 for development

### Security-Related Bug Fixes

* Fix memory leaks across 12+ modules detected via Valgrind (workflow_utils, script_handler, jws_utils, steps_handler, root_key_util, WorkflowHandle, linux_adu_core_impl, zlog, and others)
* Make communication client handle address thread-safe
* Make zlog ref_count thread-safe using atomic operations
* Fix use of atomic_bool for thread flag and add absolute timeout for response FIFO
* Fix ARM32 segfault in extension registration (incorrect format specifier %lld for long)
* Fix wrong pointer issue (#737)
* Fix all known SDK stability issues on RPI4
* Remove ADUC_Logging_Init/Uninit calls from API service thread to prevent process-wide logging corruption

### Other Bug Fixes

* Return cached version of content handler if already existing
* Fix log level being defaulted to INFO on extensions
* Fix log message to show text for cancel
* Fix missing logging Uninit
* Fix util function logging error when file does not exist
* Fix bug in ADUC_SystemUtils_CopyFileToDir
* Fix wrong documentation where -1 would read outside array
* Resolve issue in example scripts where CancelUpdate function is not invoked
* Fix line commented by mistake and remove unneeded imports
* Fix warnings and add test for ADUC_HashUtils_GetIndexStrongestValidHash
* Fix apisvc unit tests to use test data folder instead of /tmp

### Testing and Code Quality

* Extensive unit test coverage additions across adu-shell, extensions, communication_abstraction, communication_manager, agent, platform_layers, diagnostics_component, adu_types, adu_workflow, logging, and rootkey_workflow
* Add Python script and shell script for code coverage report generation
* Run unit tests with Valgrind on Ubuntu 20.04+ build hosts
* Remove Debian 10 build support (reached end of life June 2024)

## Release 1.2.0 (2024-12-16)

### Security-Related Bug Fixes

* Fix incorrect format specifier in extension_utils.c
* Fix segmentation fault in OnShutdownSignal
* Fix double-free issue in FileInfoUtils
* Fix buffer overrun and other UB when format-specifier-like string exists in script handler script
* Fix quoting issue leading to 0 args for format string with one format specifier in rootkey_workflow
* Fix 15 instances of "cmp narrow with wide in loop condition issues" across 11 files
* Fix Non-const fmt str in script_handler.cpp
* Fix possible buffer overrun by using strncmp in command_helper.c and operation_id_utils.c
* Cast ssize_t to size_t after >0 check

### Other Bug Fixes

* Ensure downloaded script handler script has correct ownership/permissions and improve adu-shell errors
* Fix restart race by adding signal handler in adu-shell
* Handle no-deployment workflowId case
* Fix message processing context init, usage, cleanup in d2c_messaging
* Fix memleak in url_utils and other rootkey util and rootkeypackage download fixes

### Usage Enhancements

* Fix incorrectly mapped errno in how-to-troubleshoot-guide.md
* Add public GitHub Actions — builds Debian 11 & 12, Ubuntu 20.04 and 22.04 × x86_64, arm64
* Fix setup_container.sh health check permissions for docker
* Fix daemon CMakeLists.txt and install.sh
* Fix aziot unix user/group and aziot services restart in debian cpack postrm
* Add support for building Debian 12 in install-deps.sh
* Update documents and scripts for multi component update examples
* Fix shellcheck error in demo script

### Code Enhancements

* Report InProgress only once when processing update deployment
* Add more rootkey logging and handle SignatureToJsonValue failure
* Upgrade Catch2 version from v2 to v3.8.0
* Allow use of curl to download rootkey package

## Release 1.1.0 (2023-12-22)

### New Features

* Add support for service-driven management of certificate root keys, establishing the trust relationship to ADU services
* Build pipeline support for Ubuntu 22.04 (x64, ARM64) and Debian 11 (x64, ARM64, ARM32)

### Security-Related Bug Fixes

* Update to C-SDK version `0.8.2023` and add support for OpenSSL 3.0

### Usage Enhancements

* Add facilitating scripts to produce deltas of docker images, supporting delta-update preparation
* Simplify how the SW Update handler interops with the underlying SW Update script
* Deprecate `swupdate` v1 handler — replaced by v2 which exposes more swupdate functionality
* Make the `/tmp` directory compile-time configurable
* Provide compile-time and runtime overrides for default paths of agent config file and data folder
* Configuration file updates
* Add ability to conditionally build multi-step handlers (`--step-handlers`)
* Cross-process communication fixes (shell and main process) for handling failures
* Documentation updates (Doxygen and README files), enhancements, and corrections

### Code Enhancements

* Memory allocation and safe-usage fixes
* Add compiler failure on usage of unsafe string-copy methods
* Update to C++17
* Reduce zlog logger in-memory buffer usage to 41 KB
* Merge several community contributions (#385, #386, #388, #390)

## Release 1.0.2 (2023-02-08)

### Bug Fixes

* Correct shared-object dependency on `libdeliveryoptimization.so.0` in DU agent .deb package and `AducIotAgent` binary (#344)
* Improve `IoTHub_CommunicationManager_Init` and `DeInit` (#363)
* Improve Simulator Update Handler documentation (#359)
* Correct result code and extended result code when agent is missing required handlers (#343)
* Improvements for SWUpdate V2 handler and script handler for installed criteria (#366)
* Improvements to script handler and SWUpdate V2 handler docs (#366)
* Correct unit tests for SWUpdate V2 handler (#366)

## Release 1.0.1 (2023-01-17)

### Bug Fixes

* Fix image-based update scenario where the device doesn't reboot after the new image is installed
* Fix deadlock in zlog flush deinit
* Fix regression in ADUC Logging init that prevented the agent from starting
* Fix memory leaks
* Fix `GetConnectionInfoFromConnectionString` to handle failures
* Fix overwrite of contract major version in downloaders and component enumerator
* Enable error handling when download handler plugin symbol is missing
* Fix install dependencies (`b36cd3d`)
* Correct the cached work folder used when processing replacement workflows
* DO `bootstrap.sh` fixed to run on Hyper-V Ubuntu 20.04 LTS (do-client #145)

### Enhancements

* Update contributor guide; enable GitHub ideas and discussions
* Remove `curl` dependency in `https_proxy_utils`
* Agent will now reconnect to IoT Hub on bad credential or expired SAS token

## Release 1.0.0 (2022-11-01)

This is the **General Availability** (GA) release of Device Update for IoT Hub. It introduces the SWUpdate V2 handler, the (preview) Microsoft Delta Download Handler, the Download Handler extensibility point, and adds support for Update Manifest v5 — enabling a clean upgrade path from Public Preview Refresh to GA.

### New Features

* New SWUpdate Handler V2 — see [SWUpdate Handler V2](src/extensions/step_handlers/swupdate_handler_v2/README.md)
* New Delta Download Handler (preview) — see [delta downloads](https://learn.microsoft.com/en-us/azure/iot-hub-device-update/device-update-multi-step-updates)

### Notable Bug Fixes

* For non-`adu` user/group, remove read and execute permissions on the ADU conf directory and read permissions on config files (least-privilege hardening)
* Allow Retry/Replacement to process workflow after a failure
* Fix OTA DU agent upgrade failure due to mismatched DO Agent version
* Fix cancel flow in steps handler
* Fix `retry-update` command becoming a no-op when previous attempt succeeded
* Clean up current workflow sandbox before processing deferred / replacement workflow
* Replace `0` ERC in script handler script result file for discoverability
* Fix off-by-one error in file info utils of diagnostics component
* Fix segfault in swupdate handler when `installedCriteria` property is missing

### Enhancements

* Improved Device-to-Cloud message reliability
* Support both Update Manifest v4 and v5 — improves PPR → GA upgrade path
* Add `shellcheck` and `clang-format` pre-commit git hooks
* MCU: implement `--command` support so other processes can send `retry-update` to the main agent
* Add Download Handler extensibility point + Microsoft Delta Download Handler
* Add EIS SAS Token Refresh
* Replace `interfaceId` with `contractModelId`; new value `dtmi:azure:iot:deviceUpdateContractModel;2`
* Move to Delivery Optimization v1.0.0
* Add extension contract version support
* Allow setting MQTT or MQTT-over-WebSockets transport protocol from `du-config`
* Allow custom compat properties to be set from `du-config`
* Add `result.h` generation from `result`
* Rearranged directory structure / renamed directories for consistency
* E2E test pipeline changes
* Build pipeline changes

## Release 0.8.2 (2022-06-27)

### Bug Fixes

* Fix reboot and restart logic regression
* Fix supply-chain warnings (#191)

### Notable Enhancements

* Add build pipeline support for Debian 10 amd64 (#191)

## Release 0.8.1 (2022-05-19)

### Bug Fixes

* Ignore duplicate workflow id after it last succeeded
* Fix `installedCriteria` error in example proxy updates
* Add missing `goto done` when APT package install fails
* Add `ADUC_Logging_Init()` to Script Content Handler
* Fix memory leak (#80)
* Fix incorrect root key modulus for `200703.R.T`
* Fix memory leak in `GetReportingJsonValue()` — free root JSON_Value when done (#157)
* Fix `postrm` failed-upgrade health check, 0.8.0 → 0.8.x
* Add visibility into `resultDetails` from content handler when content is already installed

### Notable Enhancements

* Add build pipeline support for Ubuntu 20.04
* Disable automatic component detection
* Add new Script Handler documentation
* Improve multi-component update example, Steps Handler, and Script Handler

## Release 0.8.0 (2022-02-03)
### Major changes
* Proxy update plug in/out component support
* Multi-step update capabilities for all update types
* Diagnostics
* Script Update Handler
* Add "Goal state" agent-based update orchestration
* Extensibility and Bundle Update support

### Breaking changes
* Remove Cloud-based update orchestration
* Replace adu-conf.txt config file with du-config.json
* Replace simulator platform layer with JSON-driven simulator handler

### Notable enhancements
* Update dependencies for Ubuntu 18.04 ARM64 build
* Add BASH script support for import manifest schema v4.0
* PowerShell scripts for generating Import Manifest V4 schema
* Add Adu shell trusted User permission check
* Add support for the AIS field gatewayHost for Nested Edge GatewayHostName

### Notable fixes
* Fix DO dependency breaking changes integration
* Fix Child process doesn't terminate if execvp fails
* Fix leak zlog file handle
* Fix DU Agent - File download failed if target already exist in the sandbox (erc:0xC00D0011)
* Fix syslog group doesn't exist in Debian system
* Fix Need to include our license in Debian package
* Fix zlog stops flushing to file after first roll of logs
* Fix adu-shell should exit if launched by non-adu group user

## Release 0.7.0 (2021-05-14)

*  Fix DO group permissions to allow writing connection string
*  Advance c-sdk version to include the nested edge gateway fix
*  Fix Deployment stuck at ‘in progress’ in startup code path
*  Remove scripts and tools directories from pipeline build exclusion
*  Relax special chars restrictions on DeviceInfo
*  LogLevel should include DEBUG in main.c
*  Fixed shellcheck errors in create-adu-import-manifest.sh
*  Fixed and checked file padding
*  Adu update bash script
*  Update page 'How to run agent' with latest feedback
*  Fixing erroneous padding in the arm64 pipeline yaml file
*  Remove libcpprest dependency in ADUC
*  Replace alias or name


## Release 0.7.0-rc1 (2021-03-31)
* Add module id support.
* Change idtype to default to 'module' during IoT Identity Service principal generation
* Default to config file when unable to provision with the IoT Identity Service
* Support agentRestartRequired APT Manifest property
* Add build instructions for agent to provision with IoT Identity Service
* Add generic error handlers
* Add trusted cert option for Nested Edge

## Release 0.6.0 (2021-03-02)

* Initial public release
