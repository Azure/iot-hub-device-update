# DeviceUpdate Docker Image

## Description

build_docker_image.sh is a self-contained script that builds a fairly minimal docker image that runs a subset of the DU binary Agent.

The purpose is for composing the DU Agent core with other extensions to build a dockerfile from those layers.
This allows for a minimal container image based on needs of the image.
For example, if all that is needed is a core agent that can handle a v4 update manifest and can handle APT package update content, then one can specify that and the resulting docker image will only contain just enough to support it.

## Limitations

The DU Agent (/usr/bin/AducIotAgent) in the container image can be setup to include the following extensibility points:

- Content Downloader(one of following)
  - download update content using libcurl
  - (Not available yet) download update content using DeliveryOptimization (DO)
- Update Content Handler
  - APT Package-based updates
  - (Not yet available) Simulator update content handler
  - (Not yet available) Script handler updates
  - (Not yet available) SWUpdate V2 handler
- Download handler
  - (Not yet available) Microsoft Delta Update Download handler
- Connectivity
  - get connection string from du-config.json (NOT from AIS)

## Pre-requisites

- [install docker](https://docs.docker.com/engine/install/ubuntu/)
- create a separate file with the connection string
  - can create it from existing /etc/adu/du-config.json by doing:

  ```sh
  grep connectionData /etc/adu/du-config.json | \
      cut -d\" -f 4 > /path/to/connection_string
  ```

## Defaults

By default, it will use the following defaults if not overridden:

### du config

```json
manufacturer: contoso
model: toaster
```

### container config

```json
image version: 1.0
base image: public.ecr.aws/lts/ubuntu:18.04_stable
tag duagentaptcurlubuntu18.04stable:1.0
```

### Print script help usage

Use `-h` or `--help` for usage options.

## Build the docker image

To build the docker image, run:

```sh
$ scripts/docker/build_docker_image.sh -c /path/to/connection_string --clean
```

### list docker images

```sh
$ docker images
```

## Run Docker Image

```sh
$ docker run -i -t duagentaptcurlubuntu18.04stable:1.0
...
```

## Attach to Docker Image and run bash shell

```sh
$ docker run -i -t duagentaptcurlubuntu18.04stable:1.0 bash
```
