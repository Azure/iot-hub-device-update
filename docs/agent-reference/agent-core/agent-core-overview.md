# Device Update Agent Core Library
## Quick Jump
- [Design Goals](#design-goals)
## Design Goals
- Support multiple update agent processes. See [Multiple Update Agent Processes](#multiple-update-agent-processes)
- Improve agent configurability. See [Configuration Manager](./configuration-manager.md)



## Multiple Update Agent Processes
To support an update model where each component on a device represented by an individual Twin (either Module Twin or Device Twin), Device Update Agent systemd service will create one child process per Twin connection.  
This

