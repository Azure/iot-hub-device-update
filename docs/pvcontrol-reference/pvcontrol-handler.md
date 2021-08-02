# PVControl Handler

The handler is relatively simple and serves as the glue between the ADU agent and Pantavisor. The update is contained in the manifest json that points to a tarball. That tarball is installed and applied with the help of [pvcontrol](https://gitlab.com/pantacor/pv-platforms/pvr-sdk/-/blob/master/files/usr/bin/pvcontrol) script, which performs the necessary requests to the Pantavisor engine.
