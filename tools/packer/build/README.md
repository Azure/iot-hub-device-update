# Build Container for Debian 11 stable

The .HCL will build a docker image that contains the results of building DU agent, its unit test, and the debian package.

## How to run Packer to build the Dockerfile

### Build only arm64 debian 11

packer build -only=duagent.docker.debian_arm64 .

### Build all arch (arm64 and amd64) debian 11

packer build .

## Build Artifacts Produced from packer build

The `packer build .` will result in the following build artifacts in the docker container:
- /usr/local/lib/ artifacts
  - deviceupdate-openssl
  - libparson.a
  - libumock_c.a
  - libaziotsharedutil.a
  - libumqtt.a
  - libuhttp.a
  - libiothub_client_http_transport.a
  - libiothub_client_mqtt_ws_transport.a
  - libiothub_client_mqtt_transport.a
  - libiothub_client.a
  - libserializer.a
  - libgtest.a
  - libgmock.a
  - libgmock_main.a
  - libgtest_main.a
  - libdeliveryoptimization.so.1.0.0
  - libdeliveryoptimization.so.1 -> libdeliveryoptimization.so.1.0.0
  - libdeliveryoptimization.so -> libdeliveryoptimization.so.1
  - libazure-core.a
  - libazure-security-attestation.a
  - libazure-identity.a
  - libazure-security-keyvault-keys.a
  - libazure-security-keyvault-secrets.a
  - libazure-security-keyvault-certificates.a
  - libazure-storage-common.a
  - libazure-storage-blobs.a
  - libazure-storage-files-datalake.a
  - libazure-storage-files-shares.a
  - libazure-storage-queues.a
  - libazure-template.a
  - libazure_blob_storage_file_upload_utility.a

- /iot-hub-device-update/out/ artifacts
  - deviceupdate-agent_1.0.2_arm64.deb

## The resultant docker image

$ docker image ls
REPOSITORY                   TAG            IMAGE ID       CREATED         SIZE
<none>                       <none>         112a56228b1a   5 hours ago     2.52GB
arm64v8/debian               11             2c4d303cc7f7   2 weeks ago     118MB

The correct IMAGE ID would be 112a56228b1a in the above.

## How to run the unit tests in the docker container

docker run -it --platform=linux/arm64 --rm --name container 112a56228b1a
cd /iot-hub-device-update/out
ctest

## How to get the artifacts off of docker container

docker run -it --platform=linux/arm64 --rm --name container 112a56228b1a
bash
apt-get install -y rsync
pushd / && tar czvf ./iot-hub-device-update/out/* /usr/local/lib/* ./device-update-debian11-arm64-build-artifacts.tgz
rsync -av --progress ./device-update-debian11-arm64-build-artifacts.tgz user@myhost:~/

