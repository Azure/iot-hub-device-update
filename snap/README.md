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
2. deviceupdate-agent

### Layouts
With layouts, you can make elements in $SNAP, $SNAP_DATA, $SNAP_COMMON accessible from locations such as /usr, /var and /etc. This helps when using pre-compiled binaries and libraries that expect to find files and directories outside of locations referenced by $SNAP or $SNAP_DATA.
Note that $SNAP is read-only, while $SNAP_DATA is a modifiable data folder.

