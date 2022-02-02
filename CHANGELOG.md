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
