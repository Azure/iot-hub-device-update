# Device Update Agent SNAP Devcelopment Design

## Terminology

| Achronym | Description | Note|
|---|---|---|
| DO | Delivery Optimization | e.g., DO Clent, DO Client Snap, DO SDK
| DU | Device Update | e.g., DU Agent, DU Agent Snap, DU Service


## Ubuntu Core

Ubuntu Core is a minimalist version of the Ubuntu operating system designed for use in embedded devices and Internet of Things (IoT) devices. It is based on the same Ubuntu operating system as desktop and server versions, but with a much smaller footprint and a focus on security, reliability, and easy management.

Ubuntu Core uses a "snap" packaging system to deliver software, which allows for easy installation, updates, and rollbacks of individual packages and applications. This makes it easier to manage and maintain embedded devices that may not have the same resources or capabilities as traditional computing devices.

In addition to the snap packaging system, Ubuntu Core also includes a number of security features, such as app isolation and containerization, to help ensure that the system and its applications remain secure and protected from attacks. These security features make Ubuntu Core a popular choice for use in IoT and other embedded devices where security is a critical concern.

Visit [Ubunt Core Official Page](https://ubuntu.com/core) for more information.

## Device Update Agent Snap for Ubuntu Core

The Device Update Agent is an important component of Ubuntu Core for managing updates and maintaining the security and reliability of devices running the operating system. There are several reasons why the Device Update Agent is necessary:

- Simplified update management: The Device Update Agent provides a simplified way to manage updates for all the applications and packages installed on a device running Ubuntu Core. It ensures that updates are installed in a timely and efficient manner, and helps to prevent errors or conflicts that can arise when updating software manually.

- Automatic updates: The Device Update Agent can be configured to automatically download and install updates as soon as they become available. This ensures that devices are always up to date with the latest security patches and bug fixes, without requiring any manual intervention from users or administrators.

- Security: Keeping software up to date is essential for maintaining the security of devices running Ubuntu Core. The Device Update Agent provides an additional layer of security by verifying the authenticity of updates and ensuring that only trusted packages are installed on the device.

- Rollback: In the event of a failed update, the Device Update Agent can roll back to the previous version of the software, ensuring that the device remains operational and minimizing the risk of data loss or corruption.

---

### Snap User/Group Security Model

In Ubuntu Core, each snap is packaged in a container-like environment called a snap package, which contains all of the dependencies and libraries required to run the application. The snap package is isolated from the rest of the system and runs with its own set of user and group permissions.

Here are some key points about the snap user and group security model in Ubuntu Core:

- Snap user accounts: When a snap is installed, a dedicated user account is created for that snap. The user account has its own unique UID and GID, and is used to run the snap's processes and access its resources. This helps to ensure that each snap is isolated from other snaps and from the rest of the system.

- Snap groups: In addition to the user account, snaps can also define one or more snap groups. These groups are used to manage permissions and access to system resources. For example, a snap that needs access to network resources might define a "network" snap group, and any user or process associated with that snap would need to be a member of that group in order to access network resources.

- Interfaces: To allow snaps to access resources and services outside of their own container, Ubuntu Core uses a system of interfaces. An interface is a predefined set of permissions that allows a snap to access certain system resources or services. For example, the "network" interface allows a snap to access the network, while the "camera" interface allows a snap to access the device's camera.

- AppArmor: Ubuntu Core also uses AppArmor, a Linux security module, to further isolate snaps and restrict their access to system resources. AppArmor profiles can be created for each snap, which define the set of resources that the snap is allowed to access. This helps to prevent a compromised snap from accessing or modifying other parts of the system.

Overall, the snap user and group security model in Ubuntu Core provides a high degree of isolation and security for individual applications, while still allowing them to access the resources they need to function properly.

For more information, see [Security Policy and Sandboxing of Snapcraft](https://snapcraft.io/docs/security-policy-and-sandboxing), a snap is run inside a isolated sandbox, so the user accessing Device Update Agent will always be `root`. 

#### DU Snap User and Group
Keys different between Device Update Agent and Device Update Agent Snap:

| | Device Upate Agent | Device Update Agent Snap |
|---|---|---|
| user id| adu | aziot-snap-du | 
| group id| du | aziot-snap-du |

### Snap Dependencies
There are two snaps that DU snap needs to run with.
1. Azure Identity Service: This is the snap for configuring the Edge device. Alternatively, we can use the connection string stored on du-config.json for the configuration.
2. Delivery Optimization: This is the downloader we use. Alternatively, we can use `curl` as the downloader.

#### Connect to Other Snap with Specific User ID
When connecting snaps with interfaces, the snaps are typically connected with the default user or "system" user. However, it is possible to connect snaps with a specific user ID by using the `--classic` and `--username` options with the snap connect command.

For example, to connect to Azure Identity Service with a specific user ID called "snap_aziot_adu", you would use the following command:
`snap connect --classic --username=snap_aziot_ad deviceupdate-agent:AIS-interface azureIdentityService-snap:AIS-interface`  
This will connect the two snaps using the "snap_aziot_adu" user ID, allowing the snaps to communicate with each other as that user.

It is important to note that using the `--classic` and `--username` options with the snap connect command can have security implications, as it allows the connected snaps to access each other's data and resources as the specified user. Therefore, it should only be used if necessary and with caution.

### Integrating With Another Snaps

The sections below explains how Device Update Agent snap communicate with other snap.

### Delivery Optimization Integration

To download files using DO Agent snap, the DU Agent snap need to connect to the following `slots` provided by the DO Agent snap:

```yaml
# Note: snippet from https://github.com/microsoft/do-client/blob/develop/snap/snapcraft.yaml

slots:
  port-number:
    interface: content
    content: port-number
    read: [ $SNAP_DATA/var/run/deliveryoptimization-agent ]

  # Share a folder containing sdk-config.json
  config-file:
    interface: content
    content: config-file
    write: [ $SNAP_DATA/etc/deliveryoptimization-agent/configs ]

plugs:
  client-downloads-folder:
    interface: content
    content: client-downloads-folder
    target: $SNAP_DATA/deliveryoptimization-snap-downloads-root

layout:
# adu_data_dir
  /var/lib/deliveryoptimization-snap-downloads-root:
    symlink: $SNAP_DATA/deliveryoptimization-snap-downloads-root
```

You can list a DO client snap's connections by using  the `snap connections` command. This command displays information about the snap's connections, including its slots and plugs.

```shell
adu-dev@du-ubuntu-core-2004-build:~d$ snap connections 

Interface                         Plug                                                 Slot                                     Notes
content[client-downloads-folder]  deliveryoptimization-client:client-downloads-folder  -      manual
content[do-config-file]           -                   deliveryoptimization-client:config-file  manual
content[do-port-number]           -                   deliveryoptimization-client:port-number  manual
network                           deliveryoptimization-client:network                  :network                                 -
network-bind                      deliveryoptimization-client:network-bind             :network-bind                            -

```

DU Agent snap will declare the following `plugs` and `layout`

```yaml
# Note: from ../snapcraft.yaml
plugs:
  do-port-number:
    interface: content
    content: do-port-number
    target: $SNAP_DATA/do-port-number

  do-config-file:
    interface: content
    content: do-config-file
    target: $SNAP_DATA/do-config-file


# Provides resources to be accssed by other snaps.
slots:
  #
  # A downloads folder.
  #
  # Consumer(s):
  #
  #    Delivery Optimization : writes downloaded file to this folder and/or sub-folders.
  #
  downloads-folder:
    interface: content
    content: downloads-folder
    write:
        - $SNAP_DATA/data/downloads

...

layout:
# adu_data_dir
  /var/lib/adu:
    symlink: $SNAP_DATA/data

  /var/lib/deliveryoptimization-snap-downloads-root:
    symlink: $SNAP_DATA/data/downloads

# adu_conf_dir
  /etc/adu:
    symlink: $SNAP_DATA/config
# adu_shell_dir
  /usr/lib/adu:
    symlink: $SNAP/usr/lib/adu
# adu_log_dir
  /var/log/adu:
    symlink: $SNAP_DATA/log
# curl command
  /usr/bin/curl-downloader:
    symlink: $SNAP/usr/bin/curl

```

> Note that above DU's `_plug_name_:downloads-folder` identifier must match DO's `_slot_name_:client-downloads-folder` identifier 

To connect, use following commands:

```shell
sudo snap connect deviceupdate-agent:do-port-number deliveryoptimization-client:port-number

sudo snap connect deviceupdate-agent:do-config-file deliveryoptimization-client:config-file

sudo snap connect deliveryoptimization-client:client-downloads-folder deviceupdate-agent:downloads-folder
```

Verify that connections are ok:

```shell
snap connections deviceupdate-agent 

Interface                         Plug                                                 Slot                                     Notes
content[client-downloads-folder]  deliveryoptimization-client:client-downloads-folder  deviceupdate-agent:downloads-folder      manual
content[do-config-file]           deviceupdate-agent:do-config-file                    deliveryoptimization-client:config-file  manual
content[do-port-number]           deviceupdate-agent:do-port-number                    deliveryoptimization-client:port-number  manual
home                              deviceupdate-agent:home                              :home                                    -
network                           deviceupdate-agent:network                           :network                                 -
```

### Snap Parts

Parts are used to describe your application, where its various components can be found, its build and run-time requirements, and those of its dependencies.

1. installdeps (Install Dependencies)
    > Similar to how to build a debian package, we run `./scripts/install-deps.sh --install-all-deps`.

2. agent (Build the Agent)
    > To support a build for snapcraft, we add an option to build the agent with `--ubuntu-core-snap-only`. To be more specific, inside the agent, we need to make the following changes:
    > a. Currently, the user is `root` instead of `adu` or any specified desired user.
    > b. Bypassed the `healthcheck()`. More details will be discussed in `Snap User/Group Security Model`.
    > c. Exclude DeliveryOptimization as a build dependency, as it will become a separate snap.
    > d. Exclude `InitializeCommandListenerThread()`, as there is no need to register to commands inside the snap sandbox.
    > e. Only include `script handler`, exclude other handlers, as they are not needed in the snapcraft use case.


### Snap Apps

This is exposing executable components. A snap’s executable is exposed to the host system via the top-level apps section of snapcraft.yaml. Its name needs to correspond with part name responsible for staging the executable within your snap.

1. adu-shell
    > adu-shell is used for backward compatibility. For classic agent, we're using adu-shell to run some tasks elevated as 'root', but since the snap agent is already running as root, ideally, the handlers can be modify to bypass adu-shell. For example, in the reference Script Handler, we have a bool `useAduShell`, this is used to bypass adu-shell.
2. deviceupdate-agent

### Snap Hooks
Snap hooks are scripts or programs that are run at specific points during the lifecycle of a snap. These hooks allow the snap to perform certain tasks or actions at specific times, such as during installation, removal, or configuration.

1. Configure
    > A configure hook is a script that is run when a snap is configured. This allows the snap to perform any necessary tasks or actions when its configuration options are changed, such as updating its runtime behavior or modifying its environment.
    >
    > A configure hook is defined in the snapcraft.yaml file for the snap, and is specified using the configure keyword. The configure keyword should be followed by the path to the script that should be run when the snap is configured, as well as any additional options or arguments that should be passed to the script.
    >
    > When the snap is configured using the snap set command, the configure hook will be run and the "configure.sh" script will be executed. This script can then perform any necessary tasks or actions based on the configuration options that were set.
    >
    > For example, in this reference snap agent, in `../hooks/configure`, we set up the directories and files as needed and register the extensions.

More hooks can be added to the snap as needed, including `install`, `refresh`, `remove`, etc.

### Layouts

In Snapcraft, a snap layout is a way of organizing the files and directories within a snap. A snap layout defines the structure and organization of the snap's files, and specifies where the files should be placed within the snap's directory hierarchy.

A snap layout is defined in the snapcraft.yaml file for the snap, and is specified using the layout keyword. The layout keyword should be followed by a list of directories and the paths to the files that should be placed in each directory.

With layouts, you can make elements in $SNAP, $SNAP_DATA, $SNAP_COMMON accessible from locations such as /usr, /var and /etc. This helps when using pre-compiled binaries and libraries that expect to find files and directories outside of locations referenced by $SNAP or $SNAP_DATA.
Note that $SNAP is read-only, while $SNAP_DATA is a modifiable data folder.
For example, in our reference snap agent, the following table shows the file location mapping.
#### File Locations
| Classic DU Agent | Snap DU Agent |
--- | --- |
| /var/lib/adu | $SNAP_DATA/data |
| /etc/adu | $SNAP_DATA/config |
| /usr/lib/adu | $SNAP_DATA/config |
| /var/log/adu | $SNAP_DATA/log |
| /usr/bin/curl-downloader | $SNAP/usr/bin/curl |


## Building DU Snap

To build the DU Snap, run `snapcraft` command at project root folder.

```bash
cd project_root
snapcraft --debug
````

### Build Output

If build success, you can find `deviceupdate-agent_#.#_amd64.snap` at the project root directory.

### How To View the DU Snap Package Content

You can use `unsquashfs` command to extract `.snap` file.

```shell
# Run unsquashfs <snap file name>
# e.g., unsquashfs deviceupdate-agent_0.1_amd64.snap
rm -fr squashfs-root

unsquashfs deviceupdate-agent_0.1_amd64.snap

# The content will be extracted to 'squashfs-root' directory
# You can use 'tree' command to view the directory content
tree squashfs-root
```
## Run The Snap

To run a shell in the confined environment:

```shell
$ sudo snap run --shell deviceupdate-agent
```

## View Snap Logs using Journalctl

The name of the Device Update agent in the confined environment is `snap.deviceupdate-agent.deviceupdate-agent`.

To view the journal log, use following command:

```shell
$ journalctl -u snap.deviceupdate-agent.deviceupdate-agent -f --no-tail
```

To start the daemon:

```shell
$ sudo systemctl start  snap.deviceupdate-agent.deviceupdate-agent
```

To stop the daemon:

```shell
$ sudo systemctl stop  snap.deviceupdate-agent.deviceupdate-agent
```

To restart the daemon:

```shell
$ sudo systemctl restart  snap.deviceupdate-agent.deviceupdate-agent
```


## Summary

Keys different between Device Update Agent and Device Update Agent Snap:

| | Device Upate Agent | Device Update Agent Snap |
|---|---|---|
| user id| adu | aziot-snap-du | 
| group id| du | aziot-snap-du |
| downloads folder | /var/lib/adu/downloads | $SNAP_DATA/data/downloads |
| configs file | /etc/adu/du-config.json | $SNAP_DATA/configs/du-config.json|
| logs folder | /var/log/adu | $SNAP_DATA/log 
