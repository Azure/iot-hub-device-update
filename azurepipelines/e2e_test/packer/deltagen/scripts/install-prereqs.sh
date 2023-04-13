#!/bin/sh

apt-get install -y git
apt-get install -y \
    apt-utils \
    curl \
    openssl \
    unzip \
    || exit 1

# Install .net core 5.0 sdk for Delta Generation Tool

# First install tzdata dep non-interactively to prevent interactive blocking
DEBIAN_FRONTEND=noninteractive apt-get install -y tzdata || exit 1
dpkg-reconfigure -f noninteractive tzdata || exit 1

apt-get install -y dotnet-sdk-5.0 || exit 1
