# How To Build the Device Update Agent Snap (for Ubuntu 20.04)

> NOTE: This document is a work-in-progress

## Prerequisites
- Learn about Snapcraft here https://snapcraft.io/docs/getting-started#heading--learning
- Learn about `snapcraft.yaml` schema here https://snapcraft.io/docs/snapcraft-yaml-reference
- 

## Build Environment
- Ubuntu 20.04
    - The Ubuntu 20.04 image can be found at https://releases.ubuntu.com/20.04/
- snapd, snapcraft, and [multipass](https://snapcraft.io/docs/build-options) packages
    - `snapd` is pre-installed on Ubuntu 20.04 LTS by default. For more information about `snapd`, see https://snapcraft.io/docs/installing-snapd
    - To install `snapcraft` use following command:
        ```sh
        sudo snap install snapcraft --classic
        ```
    - To install `multipass` use following command:
        ```sh
        sudo snap install multipass --classic
        ```
        For more infomation about `Multipass`, see https://multipass.run/
- KVM enabled
    - To enable KVM on a Linux Hyper-V VM, run following Powershell (as an Administrator) command on host PC
        ```powershell
        Set-VMProcessor -VMName <VMName> -ExposeVirtualizationExtensions $true
        ```
        For more detail, see [Windows 10 Nested Virtualization](https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/user-guide/nested-virtualization)

## Build Instruction

```sh
# change directory to iot-hub-device-update project root directory
pushd <project root>

# run 'snapcraft' to build the snap package using <project root>/snap/snapcraft.yaml
#
# Note: if a build error occurs, you can run snapcraft --debug to troubleshoot the error.
snapcraft

```

### Build Output

If build success, you can find `deviceupdate-agent_#.#_amd64.snap` at the project root directory.

### How To View Snap Packge Content

You can use `unsquashfs` command to extract `.snap` file.

```sh
# Run unsquashfs <snap file name>
# e.g., unsquashfs deviceupdate-agent.0.1_amd64.snap
unsquashfs deviceupdate-agent.0.1_adm64.nsap

# The content will be extracted to 'squashfs-root' directory
# You can use 'tree' command to view the directory content
tree squashfs-root
```

## Install The Snap

To install the snap for testing (developer mode)

```sh
sudo snap install --devmode ./deviceupdate-agent_0.1_amd64.snap 
```

> NOTE | This will installs the snap content to /snap/<'snap name'>/ directory on the host device

## Run The Snap

To run a shell in the confined environment

```sh
 snap run --shell deviceupdate-agent
```

Inspect some snap variables:

```sh
root@myhost:~# printenv SNAP
/snap/deviceupdate-agent/x2

root@myhost:~# printenv SNAP_COMMON
/var/snap/deviceupdate-agent/common

root@myhost:~# printenv SNAP_DATA  
/var/snap/deviceupdate-agent/x2
```

## View Snap Logs using Journalctl

The name of the Device Update agent in the confined environment is `snap.deviceupdate-agent.deviceupdate-agent`.

To view the journal log, use following command:

```sh
journalctl -u snap.deviceupdate-agent.deviceupdate-agent -f --no-tail
```


## Configure The Snap
[TBD]

## Testing The Snap
[TBD]
