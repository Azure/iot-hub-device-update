# SNAP Design Doc

## Ubuntu Core

Ubuntu Core is a secure, application-centric IoT OS for embedded devices(https://ubuntu.com/core)

## Device Update Snap for Ubuntu Core

To support Device Update for IoT Hub on Ubuntu Core, we need to build a snap.

### Snap Dependency
There are two snaps that DU snap needs to run with.
1. Azure Identity Service: This is the snap for configuring the Edge device. Alternatively, we can use the connection string stored on du-config.json for the configuration.
2. Delivery Optimization: This is the downloader we use. Alternatively, we can use `curl` as the downloader.


### Snap User/Group Security Model

Referencing to the Security Policy and Sandboxing of Snapcraft(https://snapcraft.io/docs/security-policy-and-sandboxing), a snap is run inside a isolated sandbox, so the user accessing Device Update Agent will always be `root`. Also, currently Snapcraft does not support creating a user or group like creating them in a Linux environment. This is why the security model of verifying uid and gid in Linux is not applicable in the snapcraft build for Ubuntu Core.

### Connect to Other Snap with Specific User ID
When connecting snaps with interfaces, the snaps are typically connected with the default user or "system" user. However, it is possible to connect snaps with a specific user ID by using the `--classic` and `--username` options with the snap connect command.

For example, to connect to Azure Identity Service with a specific user ID called "snap_aziot_adu", you would use the following command:

`snap connect --classic --username=snap_aziot_ad deviceupdate-agent:AIS-interface azureIdentityService-snap:AIS-interface`
This will connect the two snaps using the "snap_aziot_adu" user ID, allowing the snaps to communicate with each other as that user.

It is important to note that using the `--classic` and `--username` options with the snap connect command can have security implications, as it allows the connected snaps to access each other's data and resources as the specified user. Therefore, it should only be used if necessary and with caution.

### Snap Parts

Parts are used to describe your application, where its various components can be found, its build and run-time requirements, and those of its dependencies.

1. installdeps (Install Dependencies)
Similar to how to build a debian package, we run `./scripts/install-deps.sh --install-all-deps`.

2. agent (Build the Agent)
To support a build for snapcraft, we add an option to build the agent with `--ubuntu-core-snap-only`. To be more specific, inside the agent, we need to make the following changes:
    a. Currently, the user is `root` instead of `adu` or any specified desired user.
    b. Bypassed the `healthcheck()`. More details will be discussed in `Snap User/Group Security Model`.
    c. Exclude DeliveryOptimization as a build dependency, as it will become a separate snap.
    d. Exclude `InitializeCommandListenerThread()`, as there is no need to register to commands inside the snap sandbox.
    e. Only include `script handler`, exclude other handlers, as they are not needed in the snapcraft use case.


### Snap Apps

This is exposing executable components. A snap’s executable is exposed to the host system via the top-level apps section of snapcraft.yaml. Its name needs to correspond with part name responsible for staging the executable within your snap.

1. adu-shell
adu-shell is used for backward compatibility. For classic agent, we're using adu-shell to run some tasks elevated as 'root', but since the snap agent is already running as root, ideally, the handlers can be modify to bypass adu-shell. For example, in the reference Script Handler, we have a bool `useAduShell`, this is used to bypass adu-shell.
2. deviceupdate-agent

### Snap Hooks
Snap hooks are scripts or programs that are run at specific points during the lifecycle of a snap. These hooks allow the snap to perform certain tasks or actions at specific times, such as during installation, removal, or configuration.

1. Configure
A configure hook is a script that is run when a snap is configured. This allows the snap to perform any necessary tasks or actions when its configuration options are changed, such as updating its runtime behavior or modifying its environment.

A configure hook is defined in the snapcraft.yaml file for the snap, and is specified using the configure keyword. The configure keyword should be followed by the path to the script that should be run when the snap is configured, as well as any additional options or arguments that should be passed to the script.

When the snap is configured using the snap set command, the configure hook will be run and the "configure.sh" script will be executed. This script can then perform any necessary tasks or actions based on the configuration options that were set.

For example, in this reference snap agent, in `../hooks/configure`, we set up the directories and files as needed and register the extensions.

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
