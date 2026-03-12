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
