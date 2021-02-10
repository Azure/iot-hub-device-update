# Yocto Layers, Recipes, and Configuration

There are three main areas that will be covered:

* Yocto build configurations
* Meta-azure-device-update recipe
* Image install with A/B partitions

## Yocto build configurations

This section describes the most important files of the Yocto build, including
'bblayers.conf.sample' and 'local.conf.sample' as well as the Yocto layers that need to be used.

| Layer Name      | Description |
| ------------- | ---------- |
| meta-azure-device-update | Provides the configuration and contains the recipes for installing both the ADU Agent and its dependencies as well as integrating them into the Yocto build layers.|
| meta-openembedded   | Brings in the openembedded layer for strengthening the raspberrypi BSP. Implements some of the Core functionality. |
| meta-raspberrypi   | Implements the BSP layer for the RaspberryPi. Without this layer Yocto cannot be built to work on the raspberry pi. |
| meta-swupdate   |  Adds support for a deployment mechanisms of Yocto's images based on swupdate project. |

#### bblayers.conf.sample

The 'bblayers.conf.sample' shows the complete list of Yocto layers included in
the build.

```shell
POKY_BBLAYERS_CONF_VERSION = "3"

BBPATH = "${TOPDIR}"
BBFILES ?= ""

BBLAYERS ?= " \
  ##OEROOT##/meta \
  ##OEROOT##/meta-poky \
  ##OEROOT##/meta-yocto-bsp \
  ##OEROOT##/../meta-openembedded/meta-oe \
  ##OEROOT##/../meta-openembedded/meta-multimedia \
  ##OEROOT##/../meta-openembedded/meta-networking \
  ##OEROOT##/../meta-openembedded/meta-python \
  ##OEROOT##/../meta-raspberrypi \
  ##OEROOT##/../meta-swupdate \
  ##OEROOT##/../meta-azure-device-update \
  "
```

#### local.conf.sample

Similarly, the 'local.conf.sample' handles the configuration for machine and
package information, architecture, image types, and more.

```shell
CONF_VERSION = "1"

MACHINE ?= "raspberrypi3"
DISTRO ?= "poky"
PACKAGE_CLASSES ?= "package_ipk"
SDKMACHINE = "x86_64"
USER_CLASSES ?= "buildstats image-mklibs image-prelink"
PATCHRESOLVE = "noop"
SSTATE_DIR ?= "build/sstate-cache"

RPI_USE_U_BOOT = "1"
ENABLE_UART = "1"

IMAGE_FSTYPES += "wic wic.bmap"

# Set PREFERRED_PROVIDER_u-boot-fw-utils to prevent warning about
# having two providers of u-boot-fw-utils
PREFERRED_PROVIDER_u-boot-fw-utils = "libubootenv"

DISTRO_FEATURES_append = " systemd"
DISTRO_FEATURES_BACKFILL_CONSIDERED += "sysvinit"
VIRTUAL-RUNTIME_init_manager = "systemd"
VIRTUAL-RUNTIME_initscripts = "systemd-compat-units"
```

## Meta-azure-device-update layer

This meta-azure-device-update layer describes the Yocto recipes in the layer
that builds and installs the ADU Agent code.

#### layer.conf

It is in the layer.conf file where configuration variables are set for ADU
software version,  SWUpdate, manufacturer, and model.

**NOTE:** For private preview, manufacturer and model defined in layer.conf will
define the PnP manufacturer and model properties.  Manufacturer and model must
match the 'compatibility' in the import manifest of the [ADU Publishing
flow](../quickstarts/how-to-import-quickstart.md), with the specific format of
"manufacturer.model" (case-sensitive).

```shell
BBPATH .= ":${LAYERDIR}"

# We have a recipes directory containing .bb and .bbappend files, add to BBFILES
BBFILES += "${LAYERDIR}/recipes*/*/*.bb \
            ${LAYERDIR}/recipes*/*/*.bbappend"

BBFILE_COLLECTIONS += "azure-device-update"
BBFILE_PATTERN_azure-device-update := "^${LAYERDIR}/"

# Pri 16 ensures that our recipes are applied over other layers.
# This is applicable where we are using appends files to adjust other recipes.
BBFILE_PRIORITY_azure-device-update = "16"
LAYERDEPENDS_azure-device-update = "swupdate"
LAYERSERIES_COMPAT_azure-device-update  = "warrior"


# Layer-specific configuration variables.
ADU_SOFTWARE_VERSION ?= "0.0.0.1"

HW_REV ?= "1.0"
MANUFACTURER ?= "Contoso"
MODEL ?= "ADU Raspberry Pi Example"

BBFILES += "${@' '.join('${LAYERDIR}/%s/recipes*/*/*.%s' % (layer, ext) \
               for layer in '${BBFILE_COLLECTIONS}'.split() for ext in ['bb', 'bbappend'])}"
```

### Build ADU Agent into image

The ADU reference agent uses the
[CMake](../../cmake) build
system to build the source code binaries.  Build options are listed within the
azure-device-update_git.bb recipe and 'ADUC_SRC_URI' points to the ADU
reference agent to pull it into the image.

#### azure-device-update_git.bb

At runtime, 'RDEPENDS' lists the components that are depended on at runtime.

```shell
RDEPENDS_${PN} += "azure-device-update deliveryoptimization-agent-service"
```

In addition, there is a compile time dependency list, for the Azure IoT SDK for C and the Delivery Optimization Agent SDK.

```shell
# ADUC depends on azure-iot-sdk-c and DO Agent SDK
DEPENDS = "azure-iot-sdk-c deliveryoptimization-agent"
```

#### adu-agent-service.bb

The adu-agent-service.bb recipe is used to install the adu-agent.service that
will auto start the ADU agent service on boot for the Raspberry Pi image,
passing in the IoT Hub connection string located at /adu/adu-conf.txt
There are also similar recipes for the 'deliveryoptimization-agent' found in the
azure-device-update_git.bb bundle.

```shell
# Installs the ADU Agent Service that will auto-start the ADU Agent
# and pass in the IoT Hub connection string located at /adu/adu-conf.txt.

LICENSE="CLOSED"

SRC_URI = "\
    file://adu-agent.service \
"

SYSTEMD_SERVICE_${PN} = "adu-agent.service"

do_install_append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/adu-agent.service ${D}${systemd_system_unitdir}
}

FILES_${PN} += "${systemd_system_unitdir}/adu-agent.service"

REQUIRED_DISTRO_FEATURES = "systemd"

RDEPENDS_${PN} += "azure-device-update deliveryoptimization-agent-service"

inherit allarch systemd
```

#### adu-agent.service

The adu-agent.service is a systemd unit file that defines the ADU Agent service (adu-agent.service).
This file will be installed at /lib/systemd/system directory.

```shell
[Unit]
Description=ADU Client service.
After=network-online.target
Wants=deliveryoptimization-agent.service

[Service]
Type=simple
Restart=on-failure
RestartSec=1
User=root
# If /adu/adu-conf.txt does not exist, systemd will try to start the ADU executable
# 5 times and then give up.
# We can check logs with journalctl -f -u adu-agent.service
ExecStart=/usr/bin/AducIotAgent -l 0 -e

[Install]
WantedBy=multi-user.target
```

#### adu-device-info-files.bb

The adu-device-info-files.bb specifies files the ADU reference agent uses to
implement the Device Information PnP interface.  This generates files with ADU
applicability info for manufacturer, model and version, which can be read by the
ADU reference agent.

```shell
# Generates a text file with the ADU applicability info
# for manufacturer and model and copies/installs that file into the image.

# Environment variables that can be used to configure the behaviour of this recipe.
# MANUFACTURER          The manufacturer string that will be written to the manufacturer
#                       file and reported through the Device Information PnP Interface.
# MODEL                 The model string that wil be written to the model file and
#                       reported through the Device Information PnP Interface.
# ADU_SOFTWARE_VERSION  The software version for the image/firmware. Will be written to
#                       the version file that is read by ADU Agent.

LICENSE="CLOSED"

# Generate the manufacturer, model, and version files
do_compile() {
    echo "${MANUFACTURER}" > adu-manufacturer
    echo "${MODEL}" > adu-model
    echo "${ADU_SOFTWARE_VERSION}" > adu-version
}

# Install the files on the image in /etc
do_install() {
    install -d ${D}${sysconfdir}
    install -m ugo=r adu-manufacturer ${D}${sysconfdir}/adu-manufacturer
    install -m ugo=r adu-model ${D}${sysconfdir}/adu-model
    install -m ugo=r adu-version ${D}${sysconfdir}/adu-version
}

FILES_${PN} += "${sysconfdir}/adu-manufacturer"
FILES_${PN} += "${sysconfdir}/adu-model"
FILES_${PN} += "${sysconfdir}/adu-version"

inherit allarch
```

#### deliveryoptimization-agent_git.bb

The deliveryoptimization-agent_git.bb is a recipe for building one of ADU Agent dependencies,
Delivery Optimization Client service.

```shell
# Build and install Delivery Optimization Agent and SDK.

# Environment variables that can be used to configure the behavior of this recipe.
# DO_SRC_URI            Changes the URI where the DO code is pulled from.
#                       This URI follows the Yocto Fetchers syntax.
#                       See https://www.yoctoproject.org/docs/latest/ref-manual/ref-manual.html#var-SRC_URI
# BUILD_TYPE            Changes the type of build produced by this recipe.
#                       Valid values are Debug, Release, RelWithDebInfo, and MinRelSize.
#                       These values are the same as the CMAKE_BUILD_TYPE variable.

LICENSE = "CLOSED"

DO_SRC_URI ?= "gitsm://github.com/microsoft/do-client;branch=main"
SRC_URI = "${DO_SRC_URI}"

# This code handles setting variables for either git or for a local file.
# This is only while we are using private repos, once our repos are public,
# we will just use git.
python () {
    src_uri = d.getVar('DO_SRC_URI')
    if src_uri.startswith('git'):
        d.setVar('SRCREV', d.getVar('AUTOREV'))
        d.setVar('PV', '1.0+git' + d.getVar('SRCPV'))
        d.setVar('S', d.getVar('WORKDIR') + "/git")
    elif src_uri.startswith('file'):
        d.setVar('S',  d.getVar('WORKDIR') + "/deliveryoptimization-agent")
}

DEPENDS = "boost cpprest libproxy msft-gsl"

inherit cmake

BUILD_TYPE ?= "Debug"
EXTRA_OECMAKE += "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"
# Don't build DO tests.
EXTRA_OECMAKE += "-DDO_BUILD_TESTS=OFF"
# cpprest installs its config.cmake file in a non-standard location.
# Tell cmake where to find it.
EXTRA_OECMAKE += "-Dcpprestsdk_DIR=${WORKDIR}/recipe-sysroot/usr/lib/cmake"

BBCLASSEXTEND = "native nativesdk"

do_install_append() {
    install -d ${D}${includedir}
    install -m 0755 ${S}/include/do_config.h ${D}${includedir}/
    install -m 0755 ${S}/include/do_download.h ${D}${includedir}/
    install -m 0755 ${S}/include/do_download_status.h ${D}${includedir}/
    install -m 0755 ${S}/include/do_exceptions.h ${D}${includedir}/
}
```

#### deliveryoptimization-agent-service.bb

The deliveryoptimization-agent-service.bb file installs and configures the Delivery Optimization Client
service on the device.

```shell
# Installs and configures the DO Agent Service

LICENSE="CLOSED"

SRC_URI = "\
    file://deliveryoptimization-agent.service \
"

SYSTEMD_SERVICE_${PN} = "deliveryoptimization-agent.service"

do_install_append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/deliveryoptimization-agent.service ${D}${systemd_system_unitdir}
}

FILES_${PN} += "${systemd_system_unitdir}/deliveryoptimization-agent.service"
REQUIRED_DISTRO_FEATURES = "systemd"
RDEPENDS_${PN} += "deliveryoptimization-agent"

inherit allarch systemd

```

#### deliveryoptimization-agent.service

The deliveryoptimization-agent-list.service is a systemd unit file that defines the Delivery Optimization Client service.

```shell
[Unit]
Description=deliveryoptimization-agent: Performs content delivery optimization tasks
After=network-online.target

[Service]
Type=simple
Restart=on-failure
User=root
ExecStart=/usr/bin/docs-daemon/docs

[Install]
WantedBy=multi-user.target
```

#### azure-iot-sdk-c_git.bb

The ADU Agent communicates with ADU services using a PnP supports provied by
an Azure IoT SDK for C. azure-iot-sdk-c_git.bb is the recipe for building the SDK used by ADU Agent.

```shell
# Build and install the azure-iot-sdk-c with PnP support.

DESCRIPTION = "Microsoft Azure IoT SDKs and libraries for C"
AUTHOR = "Microsoft Corporation"
HOMEPAGE = "https://github.com/Azure/azure-iot-sdk-c"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=4283671594edec4c13aeb073c219237a"

# We pull from master branch in order to get PnP APIs
SRC_URI = "gitsm://github.com/Azure/azure-iot-sdk-c.git;branch=master"

SRCREV = "${AUTOREV}"
PV = "1.0+git${SRCPV}"

S = "${WORKDIR}/git"

# util-linux for uuid-dev
DEPENDS = "util-linux curl openssl boost cpprest libproxy msft-gsl"

inherit cmake

# Do not use amqp since it is deprecated.
# Do not build sample code to save build time.
EXTRA_OECMAKE += "-Duse_amqp:BOOL=OFF -Duse_http:BOOL=ON -Duse_mqtt:BOOL=ON -Dskip_samples:BOOL=ON -Dbuild_service_client:BOOL=OFF -Dbuild_provisioning_service_client:BOOL=OFF"

sysroot_stage_all_append () {
    sysroot_stage_dir ${D}${exec_prefix}/cmake ${SYSROOT_DESTDIR}${exec_prefix}/cmake
}


FILES_${PN}-dev += "${exec_prefix}/cmake"

BBCLASSEXTEND = "native nativesdk"
```

#### adu-base-image.bb

The adu-base-image.bb file creates an image that can be flashed on an SD card
with the ADU Agent, DO Agent, and other useful features pre-installed.

```shell
# Creates the base image for ADU that can we used to flash an SD card.
# This image is also used to populate the ADU update image.

DESCRIPTION = "ADU base image"
SECTION = ""
LICENSE="CLOSED"

# .wks file is used to create partitions in image.
WKS_FILE_raspberrypi3 = "adu-raspberrypi.wks"
# wic* images our used to flash SD cards
# ext4.gz image is used to construct swupdate image.
IMAGE_FSTYPES += "wic wic.gz ext4.gz"

IMAGE_FEATURES += "splash debug-tweaks ssh-server-openssh tools-debug tools-profile"

# connman - provides network connectivity.
# parted - provides disk partitioning utility.
# fw-env-conf - installs fw_env.config file for fw utils. (fw_*)
IMAGE_INSTALL_append = " \
    packagegroup-core-boot \
    packagegroup-core-full-cmdline \
    openssh connman connman-client \
    parted fw-env-conf \
    adu-agent-service \
    "

export IMAGE_BASENAME = "adu-base-image"

# This must be at the bottom of this file.
inherit core-image
```

#### adu-raspberrypi.wks

The adu-raspberrypi.wks file creates the partition layout and populates files in
the final ADU base image that can be flashed onto an SD card. **Note** This file
would normally update the fstab file for the ADU base image, but it would not
update the fstab file for the ADU update image. To ensure the fstab files in
both the base and update images are correct, we specify our own fstab file.

```shell
# Wic Kickstart file that defines which partitions are present in wic images.
# See https://www.yoctoproject.org/docs/current/mega-manual/mega-manual.html#ref-kickstart

# NOTE: Wic will update the /etc/fstab file in the .wic* images,
# but this file needs to also be in sync with the base-files fstab file
# that gets provisioned in the rootfs. Otherwise, updates will install
# an incompatible fstab file.

# Boot partition containing bootloader. This must be the first entry.
part /boot --source bootimg-partition --ondisk mmcblk0 --fstype=vfat --label boot --active --align 4096 --size 20 --fsoptions "defaults,sync"

# Primary rootfs partition. This must be the second entry.
part / --source rootfs --ondisk mmcblk0 --fstype=ext4 --label rootA --align 4096 --extra-space 512

# Secondary rootfs partition used for A/B updating. Starts as a copy of the primary rootfs partition.
# This must be the third entry.
part --source rootfs --ondisk mmcblk0 --fstype=ext4 --label rootB --align 4096 --extra-space 512

# ADU parition for ADU specfic configuration and logs.
# This partition allows configuration and logs to persist across updates (similar to a user data partition).
# The vfat file type allows this partition to be viewed and written to from Linux or Windows.
part /adu --ondisk mmcblk0 --fstype=vfat --label adu --align 4096 --size 512
```

#### fstab

```shell
# The fstab (/etc/fstab) (or file systems table) file is a system configuration
# file on Debian systems. The fstab file typically lists all available disks and
# disk partitions, and indicates how they are to be initialized or otherwise
# integrated into the overall system's file system.
# See https://wiki.debian.org/fstab

# Default fstab entries
/dev/root            /                    auto       defaults              1  1
proc                 /proc                proc       defaults              0  0
devpts               /dev/pts             devpts     mode=0620,gid=5       0  0
tmpfs                /run                 tmpfs      mode=0755,nodev,nosuid,strictatime 0  0
tmpfs                /var/volatile        tmpfs      defaults              0  0

# Custom fstab entries for ADU raspberrypi3.
# NOTE: these entries must be kept in sync with the corresponding .wks file.

# Mount the boot partition that contains the bootloader.
/dev/mmcblk0p1  /boot   vfat    defaults,sync   0   0

# Mount the ADU specific partition for reading configuration and writing logs.
/dev/mmcblk0p4  /adu    vfat    defaults        0   0
```

### Build image for A/B partitions

Alongside the ADU reference agent, two types of reference images, specifically
for the Raspberry Pi 3 B+ device, are built.  One is used as a base image
('rpi-u-boot-scr.bbappend', included in the raspberrypi bsp recipes) for the
initial flash of the device, installed on the "active partition" and an update
image, to be delivered by ADU, installed  on the "inactive partition".  After a
reboot the partitions will swap.

#### rpi-u-boot-scr.bbappend

The bootloader script needs to override the raspberrypi bsp layer, to handle A/B
updating.  The rpi-u-boot-scr.bbappend file tells the recipe file to look in a
different directory for the bootloader.

```shell
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
```

#### boot.cmd.in

The 'rpi-u-boot-scr.bbappend' file installs a custom 'boot.cmd.in' instead of
the default one.  This script allows the ADU Agent to change the root partition
on reboot.

```shell
saveenv
fdt addr ${fdt_addr} && fdt get value bootargs /chosen bootargs
fatload mmc 0:1 ${kernel_addr_r} @@KERNEL_IMAGETYPE@@
if env exists rpipart;then setenv bootargs ${bootargs} root=/dev/mmcblk0p${rpipart}; fi
@@KERNEL_BOOTCMD@@ ${kernel_addr_r} - ${fdt_addr}
```

#### u-boot-fw-utility_%.bbappend and u-boot-fw-utils_%.bbappend

```shell
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
```

#### fw-env-conf.bb

Similarly, the u-boot recipe file needs to be overwritten to facilitate A/B
updates.

```shell
# Copy/install fw_env.config which is necessary for fw utils (fw_*)
# like fw_printenv and fw_setenv
LICENSE = "CLOSED"

SRC_URI = "file://fw_env.config"

do_install() {
    install -d ${D}${sysconfdir}
    install -m 644 ${WORKDIR}/fw_env.config ${D}${sysconfdir}/fw_env.config
}

FILES_${PN} += "${sysconfdir}/fw_env.config"
```

### SWUpdate support

The SWUpdate framework is used to build the base and update ADU Agent images.
It is also used to install the image on the Raspberry Pi device.

#### swupdate_%.bbappend

Point to the 'defconfig' file instead using 'swupdate_%.bbappend'.

```shell
FILESEXTRAPATHS_append := "${THISDIR}/${PN}:"

PACKAGECONFIG_CONFARGS = ""
```

#### defconfig

The 'defconfig' file is how ADU configures SWUpdate to build and install into
the image, which are applied at build time.  The result is a custom
implementation of SWUpdate to suit the needs of the ADU reference agent.

```shell
CONFIG_HAVE_DOT_CONFIG=y

CONFIG_LUA=y
CONFIG_LUAPKG="lua"

CONFIG_HW_COMPATIBILITY=y
CONFIG_HW_COMPATIBILITY_FILE="/etc/adu-hw-compat"

CONFIG_CROSS_COMPILE=""
CONFIG_SYSROOT=""
CONFIG_EXTRA_CFLAGS=""
CONFIG_EXTRA_LDFLAGS=""
CONFIG_EXTRA_LDLIBS=""

CONFIG_UBOOT=y
CONFIG_UBOOT_NEWAPI=y
CONFIG_UBOOT_FWENV="/etc/fw_env.config"
CONFIG_HASH_VERIFY=y
CONFIG_SIGNED_IMAGES=y
CONFIG_SIGALG_RAWRSA=y

CONFIG_GUNZIP=y

CONFIG_LIBCONFIG=y
CONFIG_PARSERROOT=""

CONFIG_RAW=y
CONFIG_ARCHIVE=y
CONFIG_BOOTLOADERHANDLER=y
```

#### adu-update-image.bb

The adu-update-image.bb file builds the SWUpdate image.

```shell

DESCRIPTION = "ADU swupdate image"
SECTION = ""
LICENSE="CLOSED"

DEPENDS += "adu-base-image swupdate"

SRC_URI = " \
    file://sw-description \
"

# images to build before building adu update image
IMAGE_DEPENDS = "adu-base-image"

# images and files that will be included in the .swu image
SWUPDATE_IMAGES = " \
        adu-base-image \
        "

SWUPDATE_IMAGES_FSTYPES[adu-base-image] = ".ext4.gz"

# Generated RSA key with password using command:
# openssl genrsa -aes256 -passout file:priv.pass -out priv.pem
SWUPDATE_SIGNING = "RSA"
SWUPDATE_PRIVATE_KEY = "${ADUC_PRIVATE_KEY}"
SWUPDATE_PASSWORD_FILE = "${ADUC_PRIVATE_KEY_PASSWORD}"

inherit swupdate
```

#### sw-description file

This configuration file defines meta data about the update and the primary bootloader location.
Two files are referenced in the file and which partition each goes to.

```shell
copy1 on the device = location for partition A
copy2 on the device = location for partition B
```

The script looks at what partition being used.  If that partition is in use, it
will switch to using the other one.  A reboot is required to switch partitions.

```shell
software =
{
    version = "@@ADU_SOFTWARE_VERSION@@";
    raspberrypi3 = {
        hardware-compatibility: ["1.0"];
        stable = {
            copy1 : {
                images: (
                    {
                        filename = "adu-base-image-raspberrypi3.ext4.gz";
                        sha256 = "@adu-base-image-raspberrypi3.ext4.gz";
                        type = "raw";
                        compressed = true;
                        device = "/dev/mmcblk0p2";
                    }
                );
            };
            copy2 : {
                images: (
                    {
                        filename = "adu-base-image-raspberrypi3.ext4.gz";
                        sha256 = "@adu-base-image-raspberrypi3.ext4.gz";
                        type = "raw";
                        compressed = true;
                        device = "/dev/mmcblk0p3";
                    }
                );
            };
        }
    }
}
```

## Install and apply image

#### src/adu-shell/scripts/adu-swupdate.sh

This is a shell script that provides install options.

```shell
# Call swupdate with the image file and the public key for signature validation
swupdate -v -i "${image_file}" -k /adukey/public.pem -e ${selection} &>> $LOG_DIR/swupdate.log
```

#### boot.cmd.in

The boot.cmd.in looks for the 'selection' variable to know which partition to boot.

```shell
saveenv
fdt addr ${fdt_addr} && fdt get value bootargs /chosen bootargs
fatload mmc 0:1 ${kernel_addr_r} @@KERNEL_IMAGETYPE@@
if env exists rpipart;then setenv bootargs ${bootargs} root=/dev/mmcblk0p${rpipart}; fi
@@KERNEL_BOOTCMD@@ ${kernel_addr_r} - ${fdt_addr}
```
