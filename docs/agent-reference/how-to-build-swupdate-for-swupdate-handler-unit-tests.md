# How To Build swupdate for SWUpdate Handler Unit Tests

At the moment, **SWUpdate** is not available on Ubuntu 18.04. To run SWUpdate Handler unit tests, the private build of SWUpdate is required.

## About SWUpdate

See [SWUpdate Overview](https://sbabic.github.io/swupdate/swupdate.html) for more details.

Follow the following steps to build the test version of SWUpdate:

## Create Sources Folder

```sh
mkdir -p /tmp/swupdate
cd /tmp/swupdate
```

## Clone SWUpdate Project Sources

```sh
cd /tmp
git clone https://github.com/sbabic/swupdate
```

## Install Default Parser Library

SWUpdate uses the library "libconfig" as default parser for the image description. However, it is possible to extend SWUpdate and add an own parser, based on a different syntax and language as the one supported by libconfig.

See [SWUpdate: syntax and tags with the default parser](https://sbabic.github.io/swupdate/sw-description.html#swupdate-syntax-and-tags-with-the-default-parser) for more details.

For development, we need to install **libconfig-dev** package:

```sh
sudo apt install -y libconfig-dev
```

## Set Build Config for Test Version

Build configurations for swupdate can be specified in `.config` file.

For this version, we customized following options:

- CONFIG_DEFAULT_CONFIG_FILE="/usr/local/du/tests/swupdate.cfg"
- CONFIG_HW_COMPATIBILITY_FILE="/usr/local/du/tests/hwrevision"
- CONFIG_SW_VERSIONS_FILE="/usr/local/du/tests/sw-versions"
- CONFIG_DEBUG=y
- CONFIG_DEBUG_PESSIMIZE=y
- CONFIG_LIBCONFIG=y
- CONFIG_PARSERROOT="software"

**Copy the following configurations to `/tmp/swupdate/.config`**

```conf
#
# Automatically generated file; DO NOT EDIT.
# SWUpdate Configuration
#

#
# SWUpdate Settings
#

#
# General Configuration
#
CONFIG_CURL=y
# CONFIG_CURL_SSL is not set
# CONFIG_DISKFORMAT is not set
# CONFIG_SYSTEMD is not set
CONFIG_DEFAULT_CONFIG_FILE="/usr/local/du/tests/swupdate.cfg"
CONFIG_SCRIPTS=y
CONFIG_HW_COMPATIBILITY=y
CONFIG_HW_COMPATIBILITY_FILE="/usr/local/du/tests/hwrevision"
CONFIG_SW_VERSIONS_FILE="/usr/local/du/tests/sw-versions"

#
# Socket Paths
#
CONFIG_SOCKET_CTRL_PATH=""
CONFIG_SOCKET_PROGRESS_PATH=""
# CONFIG_MTD is not set
# CONFIG_LUA is not set
# CONFIG_FEATURE_SYSLOG is not set

#
# Build Options
#
CONFIG_CROSS_COMPILE=""
CONFIG_SYSROOT=""
CONFIG_EXTRA_CFLAGS="-g"
CONFIG_EXTRA_LDFLAGS=""
CONFIG_EXTRA_LDLIBS=""

#
# Debugging Options
#
CONFIG_DEBUG=y
CONFIG_DEBUG_PESSIMIZE=y
# CONFIG_WERROR is not set
CONFIG_NOCLEANUP=y

#
# Bootloader support
#
# CONFIG_BOOTLOADER_EBG is not set
# CONFIG_UBOOT is not set
CONFIG_BOOTLOADER_NONE=y
# CONFIG_BOOTLOADER_GRUB is not set
CONFIG_UPDATE_STATE_CHOICE_NONE=y
# CONFIG_UPDATE_STATE_CHOICE_BOOTLOADER is not set

#
# Interfaces
#
CONFIG_DOWNLOAD=y
# CONFIG_DOWNLOAD_SSL is not set
CONFIG_CHANNEL_CURL=y
# CONFIG_SURICATTA is not set
CONFIG_WEBSERVER=y
CONFIG_MONGOOSE=y
CONFIG_MONGOOSEIPV6=y
CONFIG_MONGOOSESSL=y

#
# Security
#
# CONFIG_SSL_IMPL_NONE is not set
CONFIG_SSL_IMPL_OPENSSL=y
# CONFIG_SSL_IMPL_WOLFSSL is not set
# CONFIG_SSL_IMPL_MBEDTLS is not set
CONFIG_HASH_VERIFY=y
# CONFIG_SIGNED_IMAGES is not set
CONFIG_ENCRYPTED_IMAGES=y
# CONFIG_ENCRYPTED_SW_DESCRIPTION is not set
# CONFIG_PKCS11 is not set

#
# Compressors (zlib always on)
#
CONFIG_GUNZIP=y
# CONFIG_ZSTD is not set

#
# Parsers
#

#
# Parser Features
#
CONFIG_LIBCONFIG=y
CONFIG_PARSERROOT="software"
# CONFIG_JSON is not set
# CONFIG_SETSWDESCRIPTION is not set

#
# Handlers
#

#
# Image Handlers
#
# CONFIG_ARCHIVE is not set
# CONFIG_BOOTLOADERHANDLER is not set
# CONFIG_DELTA is not set
# CONFIG_DISKPART is not set
# CONFIG_DISKFORMAT_HANDLER is not set
CONFIG_RAW=y
# CONFIG_RDIFFHANDLER is not set
# CONFIG_READBACKHANDLER is not set
# CONFIG_REMOTE_HANDLER is not set
CONFIG_SHELLSCRIPTHANDLER=y
# CONFIG_SWUFORWARDER_HANDLER is not set
# CONFIG_UCFWHANDLER is not set
# CONFIG_UNIQUEUUID is not set
```

## Build swupdate

```sh
cd /usr/local/du/deps/sources/swupdate && make
```

## Install swupdate to /usr/bin

```sh
cd /usr/local/du/deps/sources/swupdate && sudo make install
```
