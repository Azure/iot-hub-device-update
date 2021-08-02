# Overview

The Azure Device Update with PVControl Handler needs to be run inside a [container](https://gitlab.com/pantacor/pv-platforms/adu-agent) in a [Pantavisor-enabled](https://docs.pantahub.com/index.html) device. 

It can process updates that are similar to the ones for SWUpdate Handler, with just the manifest json pointing to a tarball that contains the pieces for the update. This is achieved with the help of the [pvr2adu](pvr2adu.md) script, which takes a [pvr checkout](https://docs.pantahub.com/make-a-new-revision.html) as input and outputs the manifest and tarball that will be consumed by Azure Device Update UI.

Internally, the [PVControl Handler](pvcontrol-handler.md) takes the tarball and installs and applies it with the help of the [pvcontrol](https://gitlab.com/pantacor/pv-platforms/pvr-sdk/-/blob/master/files/usr/bin/pvcontrol) script. This script is just a very simple HTTP client that communicates with [Pantavisor](https://docs.pantahub.com/pantavisor-commands.html), which is the core of the device and is in charge of orchestrating the running containers and managing the updates.
