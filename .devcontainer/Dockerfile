#ARG VARIANT=bookworm
ARG VARIANT=bullseye
FROM --platform=amd64 mcr.microsoft.com/devcontainers/base:${VARIANT}

WORKDIR /iot-hub-device-update

COPY ./scripts/install-deps.sh /iot-hub-device-update/

# Needed for `--install-githooks` option as part of `-a`
COPY ../.git ./.git

RUN apt-get update

RUN /bin/bash install-deps.sh -a

# To build in the container run
#./scripts/build.sh -c -u --build-packages 

# To install
# cd out && sudo cmake --build . --target install

#RUN /bin/bash /install-deps.sh --install-aduc-deps --install-do --install-cmake --install-shellcheck --work-folder /iot-hub-device-update

