# Build Container for Debian 11 using Packer

The .HCL will build a docker image that contains the results of building DU agent, its unit test, and the debian package.

## How to run Packer to build the Dockerfile

(Below was tested with Packer v1.9.1)

### Build only arm64 debian 11

```sh
packer build -only=duagent.docker.debian_arm64 .
```

## Build Artifacts Produced

The `packer build .` will result in the following build artifact in the docker container:
- /iot-hub-device-update/out/deviceupdate-agent_*_arm64.deb

It depends on deliveryoptimization-agent pkg that can be built using Dockerfile introduced with [6459cc5](https://github.com/microsoft/do-client/commit/6459cc59426d38990ed75a6f103460ae148a95db) that can be built from within vscode with the docker extension.

## Resultant docker image

```sh
$ docker image ls
REPOSITORY                   TAG            IMAGE ID       CREATED         SIZE
<none>                       <none>         112a56228b1a   5 hours ago     2.52GB
arm64v8/debian               11             2c4d303cc7f7   2 weeks ago     118MB
```

## Running the unit tests

```sh
docker run -it --platform=linux/arm64 --rm --name container 112a56
cd /iot-hub-device-update/out
ctest
```

## Get DU Agent arm64 build artifact off of docker container

```sh
docker run -it --platform=linux/arm64 --rm --name container 112a56
```

Detach with CTRL-P CTRL-Q

```sh
$ docker ps
CONTAINER ID   IMAGE          COMMAND     CREATED         STATUS         PORTS     NAMES
dc3e62308c48   112a56228b1a   /bin/sh   6 seconds ago   Up 5 seconds             container
```

Copy from container to host

```sh
docker cp dc3e623:/iot-hub-device-update/out/deviceupdate-agent_1.0.2_arm64.deb .
```

## Install onto target ARM64 device

Transfer the following from host to target and run apt-get install in that order:
- deliveryoptimization-agent_1.0.0_arm64.deb
- libdeliveryoptimization_1.0.0_arm64.deb
- deviceupdate-agent_1.0.2_arm64.deb

Installing from packages built from source is recommended until Debian 11 packages are released on packages.microsoft.com. However, for the 1st DO pkg, if installing from the [do-client main branch v1.0.0 github release](https://github.com/microsoft/do-client/releases/tag/v1.0.0) instead of from built .deb from develop branch, one may need to do local .deb install the following deps from buster in the order shown:
- [libboost-system1.67.0_1.67.0-13+deb10u1_arm64.deb](http://http.us.debian.org/debian/pool/main/b/boost1.67/libboost-system1.67.0_1.67.0-13+deb10u1_arm64.deb)
- [libboost-filesystem1.67.0_1.67.0-13+deb10u1_arm64.deb](http://http.us.debian.org/debian/pool/main/b/boost1.67/libboost-filesystem1.67.0_1.67.0-13+deb10u1_arm64.deb)

