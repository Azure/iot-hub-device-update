# DeviceUpdate Docker Image to Generate Microsoft Delta Update SWUpdate Diff Payload

## Overview

The hcl2 file in this dir for easily creating and maintaining reproducible
builds of the docker image that is used in the e2e test pipeline to generate
delta update payload. The tool used is
[Hashicorp Packer](https://developer.hashicorp.com/packer).

The scripts run by packer on the base container to set up the container image
are located in the `scripts/` directory (excluding run.sh)

## Requirements Before Running Packer

The working dir, `/opt/adu/` needs to have a linux64-diffgen-tool.zip that is created from
building
[iot-hub-device-update-delta](https://github.com/iot-hub-device-update-delta)

The path to the zip on the host can be overridden by overriding the `diffgen_tool_zip_filepath` variable can be done as per the
[assigning variables documentation](https://developer.hashicorp.com/packer/guides/hcl/variables#assigning-variables).

## Install Hashicorp Packer

Install packer using the [packer docs](https://developer.hashicorp.com/packer/tutorials/docker-get-started/get-started-install-cli).

## Initialize Packer

In this dir, run:

```sh
packer init .
```

## Build Docker Image using Packer

```sh
packer build .
```

## Confirm Existence in Docker Images

```sh
docker images
```

## Run Container to Generate Delta Update Payload

```sh
./scripts/run.sh <URL to source .swu> <URL to target .swu> /path/to/output/dir
```
