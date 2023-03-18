#!/bin/sh

#
# Taken from the following but with sudo removed:
# https://learn.microsoft.com/en-us/windows-server/administration/linux-package-repository-for-microsoft-software#examples
#
apt-get update || exit 1
apt-get install -y curl wget sudo || exit 1

# PMC APT repo config for use by .net core 6 sdk install
wget https://packages.microsoft.com/config/ubuntu/20.04/packages-microsoft-prod.deb -O packages-microsoft-prod.deb || exit 1
dpkg -i packages-microsoft-prod.deb || exit 1
rm packages-microsoft-prod.deb

# Install repository configuration
curl -sSL https://packages.microsoft.com/config/ubuntu/20.04/prod.list | tee /etc/apt/sources.list.d/microsoft-prod.list || exit 1

# Install Microsoft GPG public key
curl -sSL https://packages.microsoft.com/keys/microsoft.asc | tee /etc/apt/trusted.gpg.d/microsoft.asc || exit 1

# Update package index files
apt-get update || exit 1
