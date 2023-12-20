# Azure Build Pipelines ReadMe

## Introduction

This document is intended as a guideline to understand the structure of the current build pipeline and to add more pipelines in the future when needed.  

## Build Templates

Underneath the `templates/` directory there are two `*.yml` files.  
One is `adu-native-build-steps.yml`, because the default Azure VM is running on Ubuntu 18.04 amd64 environment, this is the current supported native build. The other one is `adu-docker-build.steps.yml`, which is a template of docker build.  
For the templates, there are two parameters: `targetOs` and `targetArch`. These parameters will be passed from build pipeline yaml files.

## Native Builds

The native build calls `scripts/install-deps.sh` and then calls `scripts/build.sh`.

## Docker Builds

The docker build pulls a container with the dependencies pre-installed from Azure Container Registry. This way, the build time can be reduced because the dependencies don't need to be installed on each build.  

## Add a New Pipeline

If you need to add support for a new platform, likely it does not have native Azure VM support (currently it only supports Ubuntu AMD64). In this case, you need to:

1. Create a docker container image with the desirable OS and architecture, clone the ADU repo
2. run `scripts/install-deps.sh` to install dependencies on this container
3. Push the image to Azure Container Registry
4. If the combination of distro & architecture already exsits in the below matrix, you would find the yaml file, and then add a `job` on that yaml file. Example: Job `BuildAduAgent_ubuntu2004` and job `BuildAduAgent_ubuntu1804` on `docker/adu-ubuntu-arm64-build.yml`.

Below is a matrix of currently supported pipeline builds
| Distro |  Architecture| Yaml File |
|--------|--------------|-----------|
| Ubuntu 18.04 | amd64 | native/adu-ubuntu-amd64-build.yml |
| Ubuntu 20.04 | amd64 | docker/adu-ubuntu-amd64-build.yml |
| Ubuntu 18.04 | arm64 | docker/adu-ubuntu-arm64-build.yml |
| Ubuntu 20.04 | arm64 | docker/adu-ubuntu-arm64-build.yml |
| Debian 10 | arm32 | docker/adu-debian-arm32-build.yml |
| Debian 10 | arm64 | docker/adu-debian-arm64-build.yml |
| Debian 10 | amd64 | docker/adu-debian-amd64-build.yml |
