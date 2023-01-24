# Tutorial - Integrate Device Update Agent into Ubuntu 20.04 arm64 for Raspberry Pi 3 & 4

This tutorial will walk you through steps for creating the **Ubuntu 20.04.5 Server Image for Raspberry Pi 3 and 4**, that include the Device Update agent for IoT Hub.

This tutorial leverage an open-source ARM devices distribution builder called [CustomPiOS](https://github.com/guysoft/CustomPiOS) to customize [ubuntu-20.04.5-preinstalled-server-arm64+raspi.img.xz](http://cdimage.ubuntu.com/releases/20.04/release/ubuntu-20.04.5-preinstalled-server-arm64+raspi.img.xz) image file to include the Device Update agent and its dependencies.

The output image can be used for demonstrating various features of the [Device Update for IoT Hub](https://learn.microsoft.com/azure/iot-hub-device-update/understand-device-update). However, main focus of this tutorial is to enable an Image-Based Update for Raspberry Pi devices by leveraging [SWUpdate Framework](http://swupdate.org/).

## Prerequisites

- Understand how CustomPiOS build tool works. See [CustomPiOS on GitHub](https://github.com/guysoft/CustomPiOS) to learn more about the tool.
- Understand how Image-Based Update works. Especially, a **"Dual-Copy"** update strategy.
- Knowledge about the [Device Update for IoT Hub](https://learn.microsoft.com/azure/iot-hub-device-update/understand-device-update).

## Development Environment Requirements

- A build computer running the Linux operating system (this tutorial is using Ubuntu 20.04.5 server).
- Base image file ([ubuntu-20.04.5-preinstalled-server-arm64+raspi.img.xz](http://cdimage.ubuntu.com/releases/20.04/release/ubuntu-20.04.5-preinstalled-server-arm64+raspi.img.xz)).
- Internet access.
- GitHub user account for cloning the [CustomPiOS](https://github.com/guysoft/CustomPiOS) project.

## Table Of Contents

- [What is an Image-Based update?](#what-is-an-image-based-update)
- [What is aDual-Copy update strategy](#what-is-a-dual-copy-update-strategy)
- [Use CustomPiOS to customize the image, step by step](#use-custompios-to-customize-the-image---step-by-step)

## What is an Image-Based Update?

An image-based update is often referred to an software update process where the where the whole partition (usually Root File System partition) is atomically replaced with a new version, instead of incrementally update part of the system (services or applications) in-place. This process is commonly used to ensure that the device is updated to the new state in which the partition is containing a set of services, applications and data that has been built and the services and applications integration has been completely tested.

This update process is commonly chosen by the solution operator when managing large fleet of devices, to ensure that every device in the cluster is operating with the same set of services and applications for consistency, predictability, and simplified manageability purposes.

## What is a Dual-Copy update strategy?

The **Dual-Copy** update is one of the update strategies where the new version of the system partition is installed into an empty partition, then the device is rebooted with a new version of the system. After rebooted, if the new version doesn't pass the update validation, the device can be rebooted with the older version of the system partition. This allows the device to be restored to the last known good state, in case of any fatal update failures.

The **Dual-Copy** strategy, as the name implies, is required 2 copies of the partitions intended to be updated (usually, Boot File System partition, Root File System partition, and Firmware partition)

In this tutorial, we will be customizing the existing Raspberry Pi image to enable Dual RootFS partitions.

## Use CustomPiOS to customize the image - step by step

> NOTE | the following instructions assumed that you have setup your build compute, which running Ubuntu 20.04.5 server or desktop.

According to the requirement for a Dual-Copy update strategy described above, we need to accomplish the tasks below:

- Create a second `RootFS` partition that has enough storage space for the future `RootFS` update. (We recommend same size as the existing `RootFS` partition).
- Create an `du data` partition that will be used to store DU related configurations and data.
- [Optional] Create an additional data partition that will be used to store any persistent application data that will survive the update.
- Customize the U-Boot boot script to support the 'Dual-Copy' update strategy.
- Integrate the Device Update agent into the image, using DEB package from `packages.microsoft.com`

### Create the Second RootFS Partition

In order to support A/B update, we need to create a secondary Root FS partition with the same storage space as the primary partition.
The secondary Root FS allows the Device Update agent to installs an **update** image onto the **non-active** partition, validates the installation result, then instructs the boot-loader to use the partition that contain the **update image** as an active Root FS.

### Device Update Data Partition
To prevent any data-loss that could affect the Device Update Agent functionality, it is recommended that the DU Data partition should be created and all DU-related persistent data should be stored in this new DU Data partition.

The following directory should be mounted appropriately:

| directory | mount target| description|
|---|---|---|
/etc/adu | /adu | contains configuration files
/var/lib/adu/downloads/ | /adu/downloads | contains downloaded content
/var/log/adu | /adu/logs | contains log files

## Modify CustomPiOS

> NOTE | Follow instructions is adapted from https://github.com/guysoft/CustomPiOS/tree/read-only-overlay#how-to-use-it

- Clone CustomPiOS GitHub Project (**this instruction is using [commit# 765784422198fd1294cdd10b591666d5407d337b](https://github.com/guysoft/CustomPiOS/tree/765784422198fd1294cdd10b591666d5407d337b)**)

    ```shell
    adu-dev@customPiOS-2204-build:~$ git clone https://github.com/guysoft/CustomPiOS custompios
    Cloning into 'custompios'...
    remote: Enumerating objects: 4558, done.
    remote: Counting objects: 100% (774/774), done.
    remote: Compressing objects: 100% (375/375), done.
    remote: Total 4558 (delta 427), reused 625 (delta 355), pack-reused 3784
    Receiving objects: 100% (4558/4558), 1.01 MiB | 8.11 MiB/s, done.
    Resolving deltas: 100% (2379/2379), done.
    ```

- Change directory to root of the project
  
    ```shell
    adu-dev@customPiOS-2204-build:~$ cd custompios
    adu-dev@customPiOS-2204-build:~/custompios$ 
    ```

- Create your distro folder by running 1src/make_custom_pi_os -g <distro_folder>1 (in this case  `distro_folder` is **ubuntu-2004-rpi-adu**)  
  
    ```shell

    adu-dev@customPiOS-2204-build:~/custompios$ src/make_custom_pi_os -g ubuntu-2004-adu
    Settings:
    making dstro in ubuntu-2004-adu
    Downloading latest Raspbian image
    % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                    Dload  Upload   Total   Spent    Left  Speed
    100  3365  100  3365    0     0   3587      0 --:--:-- --:--:-- --:--:--  3583
    % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                    Dload  Upload   Total   Spent    Left  Speed
    100  2462  100  2462    0     0   7506      0 --:--:-- --:--:-- --:--:--  7506
    % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                    Dload  Upload   Total   Spent    Left  Speed
    100  337M  100  337M    0     0  15.9M      0  0:00:21  0:00:21 --:--:-- 17.3M

    ```

    The **make_custom_pi_os** script will generate a new distro folder below:

    ```shell
    adu-dev@customPiOS-2204-build:~/custompios$ tree ubuntu-2004-adu
    ubuntu-2004-adu
    └── src
        ├── build_dist
        ├── config
        ├── custompios_path
        ├── image
        │   ├── 2022-09-22-raspios-bullseye-armhf-lite.img.xz
        │   └── README
        ├── modules
        │   └── ubuntu-2004-adu
        │       ├── config
        │       ├── end_chroot_script
        │       ├── filesystem
        │       │   ├── boot
        │       │   │   └── README
        │       │   ├── home
        │       │   │   ├── pi
        │       │   │   │   └── README
        │       │   │   └── root
        │       │   │       └── README
        │       │   └── root
        │       │       └── README
        │       └── start_chroot_script
        └── vagrant
            ├── run_vagrant_build.sh
            ├── setup.sh
            └── Vagrantfile

    11 directories, 15 files
    
    ```

- Download [ubuntu-20.04.5-preinstalled-server-arm64+raspi.img.xz](http://cdimage.ubuntu.com/releases/20.04/release/ubuntu-20.04.5-preinstalled-server-arm64+raspi.img.xz) and place it in `ubuntu-2004-adu/src/image` folder. You can delete other image file in `ubuntu-2004-adu/src/image` folder. Your distro folder should looks like this:

    ```shell
    ubuntu-2004-adu/
    └── src
        ├── build_dist
        ├── config
        ├── custompios_path
        ├── image
        │   ├── README
        │   └── ubuntu-20.04.5-preinstalled-server-arm64+raspi.img.xz
        ├── modules
        │   └── ubuntu-2004-adu
        │       ├── config
        │       ├── end_chroot_script
        │       ├── filesystem
        │       │   ├── boot
        │       │   │   └── README
        │       │   ├── home
        │       │   │   ├── pi
        │       │   │   │   └── README
        │       │   │   └── root
        │       │   │       └── README
        │       │   └── root
        │       │       └── README
        │       └── start_chroot_script
        └── vagrant
            ├── run_vagrant_build.sh
            ├── setup.sh
            └── Vagrantfile

    11 directories, 15 files
    ```

- Edit `ubuntu-2004-adu/src/config`

    ```text
    export DIST_NAME=ubuntu2004-adu
    export DIST_VERSION=0.0.1
    export MODULES="base(network,ubuntu2004-adu)"

    export BASE_DISTRO=ubuntu
    export BASE_ARCH=arm64

    export BASE_ADD_USER=yes
    export BASE_USER=pi
    export BASE_USER_PASSWORD=adupi

    export BASE_IMAGE_ENLARGEROOT=4000
    export BASE_IMAGE_RESIZEROOT=200

    # TODO: Replace 'CustomePiOS ROOT PATH' with the actual string
    export BASE_ZIP_IMG=~/custompios/ubuntu-2004-adu/src/image/ubuntu-20.04.5-preinstalled-server-arm64+raspi.img.xz

    ```

- Comment out content in `ubuntu2004-adu/src/modules/ubuntu-2004-adu`

    ```shell
    # UBUNTU-2004-ADU_VAR="This is a module variable"
    ```

- Install build dependencies

    ```shell
    sudo apt -y install jq
    ```

- Try build the image by running command `ubuntu-2004-adu/src/build_dist`. If build success, you should see the result like below:
  
    ```sh
    
    ...

    +++ losetup -d /dev/loop6
    +++ trap - EXIT
    +++ popd
    /home/adu-dev/custompios
    +++ echo_green -e '\nBUILD SUCCEEDED!\n'
    +++ echo -e -n '\e[92m'
    +++ echo -e '\nBUILD' 'SUCCEEDED!\n'

    BUILD SUCCEEDED!

    +++ echo -e -n '\e[0m'
    ```

    The output file will be located at `~/custompios/ubuntu-2004-adu/src/workspace/ubuntu-20.04.5-preinstalled-server-arm64+raspi.img`

    ```shell
    adu-dev@customPiOS-2204-build:~/custompios$ ls -l ubuntu-2004-adu/src/workspace/
    total 3422396
    drwxr-xr-x 3 root root       4096 Jan 20 06:19 aptcache
    -rwxr-xr-x 1 root root        854 Jan 20 06:16 chroot_script
    drwxr-xr-x 2 root root       4096 Jan 20 06:08 mount
    -rwxrwxrwx 1 root root 3519022080 Jan 20 06:19 ubuntu-20.04.5-preinstalled-server-arm64+raspi.img
    ```

    > NOTE | Before continue to the next step, it is a good practice to verify that the built image is working correctly. You should test the image on the Raspberry Pi device first.

### Adding additional partitions for 'Dual Copy' update strategy

Before making any changes, let's take a look at the current partitions information in the image.

cd into `ubuntu-2004-adu/src/workspace` folder then run `fdisk -l --bytes`

```shell

adu-dev@customPiOS-2204-build:~/custompios$ cd ./ubuntu-2004-adu/src/workspace/
adu-dev@customPiOS-2204-build:~/custompios/ubuntu-2004-adu/src/workspace$ fdisk -l --bytes ./ubuntu-20.04.5-preinstalled-server-arm64+raspi.img 

Disk ./ubuntu-20.04.5-preinstalled-server-arm64+raspi.img: 3.28 GiB, 3519022080 bytes, 6873090 sectors
Units: sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disklabel type: dos
Disk identifier: 0x354db354

Device                                                Boot  Start     End Sectors       Size Id Type
./ubuntu-20.04.5-preinstalled-server-arm64+raspi.img1 *      2048  526335  524288  268435456  c W95 FAT32 (LBA)
./ubuntu-20.04.5-preinstalled-server-arm64+raspi.img2      526336 6873089 6346754 3249538048 83 Linux

```

As you can see, the image currently contains 2 partitions; The **boot partition**, and the **root file system** (or **RootFS**) partition, respectively.  

As described earlier, we need to create one more RootFS partition, so that we can install an update to the inactive RootFS partition then instruct the boot-loader to use the updated partition after rebooted the device.

#### Add Second RootFS

To create the second RootFS, we need to add additional helper functions used for creating the RootFS partition based on the existing RootFS partition information.

- Add `create_rootfs_partition` and `new_partition` to **src/common.sh**

    ```sh
    function create_partition(){
        # Usage create_partition <base_image_file> <size_in_bytes> <partition_type> <file_system_type>
        image=$1
        size_bytes=$2
        new_partition_type=$3
        new_partition_fstype=$4

        echo "Creat a new partition on $image ($size_bytes bytes, $new_partition_type, $new_partition_fstype)"

        # Increase the image file size.
        echo "Adding $size_bytes bytes to $image (original size: $size_bytes)..."
        size_quo_mb=$(($size_bytes / $one_mb))
        size_rem=$(($size_bytes - ($size_quo_mb * $one_mb)))
        dd if=/dev/zero bs=$one_mb count=$size_quo_mb >> $image
        if [[ $size_bytes != 0 ]]
        then
            dd if=/dev/zero bs=1 count=$size_rem >> $image
        fi

        added_bytes=$( ls -l $image | awk '{print $5-$size_bytes}' )

        if [[ $added_bytes == $size_bytes ]]; then
            echor "Error: failed to add $size_bytes bytes to the image file. "
            EXIT
        fi

        detach_all_loopback $image
        test_for_image $image
        LODEV=$(losetup --partscan --show --find $image)

        echo "Create new partition "
        free_part=$( parted $LODEV --script unit B print free | grep "Free Space" | awk END'{print}' )
        start_mi=$( echo $free_part | awk '{print $1-0}' )
        end_mi=$( echo $free_part | awk '{print $2-0}' )
        parted $LODEV --script unit B mkpart $new_partition_type $new_partition_fstype $start_mi $end_mi
        
        # Format partition
        # Get the number of the newly created partition (by looking at the start offset, which is unique)
        new_part_info=$( parted $LODEV --script unit B print | grep $start_mi | awk END'{print}' )
        new_part_number=$( echo $new_part_info | awk '{print $1-0}' )
        part_text=p$new_part_number
        mkfs.ext4 "$LODEV$part_text"

        parted $LODEV --script unit B print

        detach_all_loopback $image
        test_for_image $image
    }

    function create_rootfs_partition(){
        # Usage create_root_partition <base_image_file> <source_partition_number> <new_partition_number> <new_partition_type> <new_partition_filesystem_type>
        image=$1
        partition=$2
        new_partition=$3
        new_partition_type=$4
        new_partition_fstype=$5

        echo "Creating new rootfs partition with the same size as partiion #$partition on $image (#$new_partition, $new_partition_type, $new_partition_fstype)"

        sector_size=512
        one_mi=$((1000 * 1000))
        one_mb=$((1024 * 1024))

        
        partitioninfo=$(sfdisk -l --bytes $image | grep "$image$partition")
        size_in_bytes=$(echo $partitioninfo | awk '{print $5-0}')

        detach_all_loopback $image
        test_for_image $image

        create_partition $image $size_in_bytes $new_partition_type $new_partition_fstype

        detach_all_loopback $image
        }
    ```

- Modify `src/custompios` script to call the new helper functions to create additional partitions. Call create_rootfs_partition() and create_partition() as needed. See example code below. Note that this should be done after enlarged the primary RootFS (**enlarge_ext** function) and before **mount_image** is called.  

    ```sh

        if [ -n "$BASE_IMAGE_ENLARGEROOT" ]
        then
            # make our image a bit larger so we don't run into size problems...
            enlarge_ext $BASE_IMG_PATH $BASE_ROOT_PARTITION $BASE_IMAGE_ENLARGEROOT
        fi


        ##### BEGIN - ADDITIONAL CODE #####
        #
        #
        if [ -n "$BUILD_OPTION_DUAL_ROOTFS" ]
        then
            # TODO: should get partition type and filesystem type from the existing RootFS partition.
            create_rootfs_partition  $BASE_IMG_PATH $BASE_ROOT_PARTITION $(($BASE_ROOT_PARTITION + 1)) "primary" "ext4"
        fi

        if [ -n "$BUILD_OPTION_CREATE_DATA_PARTITION" ]
        then
            create_partition $BASE_IMG_PATH $BUILD_OPTION_CREATE_DATA_PARTITION_SIZE_BYTES "primary" "$BUILD_OPTION_CREATE_DATA_PARTITION_FSTYPE"
        fi

        #
        ##### END - ADDITIONAL CODE #####

        # mount root and boot partition
        mount_image "${BASE_IMG_PATH}" "${BASE_ROOT_PARTITION}" "${BASE_MOUNT_PATH}" "${BASE_BOOT_MOUNT_PATH}" "${BASE_BOOT_PARTITION}"
        if [ -n "$BASE_APT_CACHE" ] && [ "$BASE_APT_CACHE" != "no" ]
        then
            mkdir -p "$BASE_APT_CACHE"
            mount --bind "$BASE_APT_CACHE" $BASE_MOUNT_PATH/var/cache/apt
        fi
    ```

    > NOTE | Set **BUILD_OPTION_DUAL_ROOTFS** and **BUILD_OPTION_CREATE_DATA_PARTITION** to "yes", and specify data partition size by setting **BUILD_OPTION_CREATE_DATA_PARTITION_SIZE_BYTES**

- Due to additional RootFS partition, we need to made small changes to **mount_image** function to mount the RootFS partition at the right offset and size. Take a look at the modified **mount_image** function below. (The added codes can be enabled by setting **BUILD_OPTION_MOUNT_UNIT="bytes"**)

    ```shell
    function mount_image() {
        image_path=$1
        root_partition=$2
        mount_path=$3
        
        boot_mount_path=boot

        if [ "$#" -gt 3 ]
        then
            boot_mount_path=$4
        fi

        if [ "$#" -gt 4 ] && [ "$5" != "" ]
        then
            boot_partition=$5
        else
            boot_partition=1
        fi

        if [ "$BUILD_OPTION_MOUNT_UNIT" == "bytes" ]; then
            # Avoid "overlapping loop device exists" error by using actual partition starting location, in bytes.
            detach_all_loopback $image_path
            test_for_image $image_path
            LODEV=$( losetup --partscan --show --find "${image_path}" )
            boot_offset=$( parted $LODEV --script unit B print | grep "^ $boot_partition" | awk '{print $2-0}' )
            boot_size_bytes=$( parted $LODEV --script unit B print | grep "^ $boot_partition" | awk '{print $4-0}' )
            root_offset=$( parted $LODEV --script unit B print | grep "^ $root_partition" | awk '{print $2-0}' )
            root_size_bytes=$( parted $LODEV --script unit B print | grep "^ $root_partition" | awk '{print $4-0}' )
        else
            # dump the partition table, locate boot partition and root partition
            fdisk_output=$(sfdisk --byte --json "${image_path}" )
            boot_offset=$(($(jq ".partitiontable.partitions[] | select(.node == \"$image_path$boot_partition\").start" <<< ${fdisk_output}) * 512))
            root_offset=$(($(jq ".partitiontable.partitions[] | select(.node == \"$image_path$root_partition\").start" <<< ${fdisk_output}) * 512))
        fi

        echo "Mounting image $image_path on $mount_path, offset for boot partition is $boot_offset, offset for root partition is $root_offset"

        # mount root and boot partition
        
        if [ "$BUILD_OPTION_MOUNT_UNIT" == "bytes" ]; then
            force_detach_all_loopback $image_path
            echo "Mounting root parition"
            sudo losetup -f
            sudo mount -o loop,offset=$root_offset,sizelimit=$root_size_bytes $image_path $mount_path/
        else
            detach_all_loopback $image_path
            echo "Mounting root parition"
            sudo losetup -f
            sudo mount -o loop,offset=$root_offset $image_path $mount_path/
        fi
        
        if [[ "$boot_partition" != "$root_partition" ]]; then
            echo "Mounting boot partition"
            sudo losetup -f
            if [ "$BUILD_OPTION_MOUNT_UNIT" == "bytes" ]; then
            sudo mount -o loop,offset=$boot_offset,sizelimit=$boot_size_bytes "${image_path}" "${mount_path}"/"${boot_mount_path}"
            else
                sudo mount -o loop,offset=$boot_offset,sizelimit=$( expr $root_offset - $boot_offset ) "${image_path}" "${mount_path}"/"${boot_mount_path}"
            fi
        fi
        sudo mkdir -p $mount_path/dev/pts
        sudo mount -o bind /dev $mount_path/dev
        sudo mount -o bind /dev/pts $mount_path/dev/pts
        sudo mount -o bind /proc $mount_path/proc
    }
    ```

- In this tutorial, we don't want to shrink the image to the minimum size. So, we added yet another build option to disable image resize feature. This can be configured by setting **BUILD_OPTION_DONOT_RESIZEROOT=yes**. See modified code in **`src/custompios`** below:

    ```shell
    if [ -n "$BASE_IMAGE_RESIZEROOT" ] && [ "$BUILD_OPTION_DONOT_RESIZEROOT" != "yes" ]; then
      # resize image to minimal size + provided size
      minimize_ext $BASE_IMG_PATH $BASE_ROOT_PARTITION $BASE_IMAGE_RESIZEROOT

      if [ -n "$BUILD_OPTION_DUAL_ROOTFS" ]; then
        # resize the 2nd RootFS partition to minimal size + provided size
        minimize_ext $BASE_IMG_PATH $(($BASE_ROOT_PARTITION + 1)) $BASE_IMAGE_RESIZEROOT
      fi
    fi
    ```

- Configure your distro accordingly. For this tutorial, your distro **config** file should look similar to this:

    ```shell
    export DIST_NAME=ubuntu-2004-adu
    export DIST_VERSION=0.1.0

    # rpi-imager json generator settings
    export RPI_IMAGER_NAME="${DIST_NAME}"
    export RPI_IMAGER_DESCRIPTION="A Raspberry Pi distro built with CustomPiOS"
    export RPI_IMAGER_WEBSITE="https://github.com/guysoft/CustomPiOS"
    export RPI_IMAGER_ICON="https://raw.githubusercontent.com/guysoft/CustomPiOS/devel/media/rpi-imager-CustomPiOS.png"

    export MODULES="base(network,ubuntu-2004-adu)"

    # Base distro information
    export BASE_DISTRO=ubuntu
    export BASE_ARCH=arm64

    # Add user
    # *** IMPORTANT *** change user and password before distributing the image.
    export BASE_ADD_USER=yes
    export BASE_USER=pi
    export BASE_USER_PASSWORD=adupi

    export BASE_IMAGE_ENLARGEROOT=2000

    # Don't resize root
    #export BASE_IMAGE_RESIZEROOT=2000
    export BUILD_OPTION_DONOT_RESIZEROOT=yes

    # Explicitly specify the image path here
    export BASE_ZIP_IMG=$DIS_PATH/image/ubuntu-20.04.5-preinstalled-server-arm64+raspi.img.xz

    # Create 2nd rootfs partition (for dual-copy update strategy)
    export BUILD_OPTION_DUAL_ROOTFS=yes

    # Create a persistent data partition for ADU configurations and data.
    export BUILD_OPTION_CREATE_DATA_PARTITION=yes
    export BUILD_OPTION_CREATE_DATA_PARTITION_SIZE_BYTES=2147483648
    export BUILD_OPTION_CREATE_DATA_PARTITION_FSTYPE=ext4

    # Mount using actual bytes offset instead of calculating from block/sector size.
    export BUILD_OPTION_MOUNT_UNIT=bytes
    ```

At this point, you should be able to built the image that contains dual RootFS partitions and extra data partition. Run `sfdisk -l --bytes PATH_TO_IMAGE_FILE` to see the image content:

```shell
adu-dev@customPiOS-2204-build:~/custompios$ sfdisk -l --bytes ./ubuntu-2004-adu/src/workspace/ubuntu-20.04.5-preinstalled-server-arm64+raspi.img 
Disk ./ubuntu-2004-adu/src/workspace/ubuntu-20.04.5-preinstalled-server-arm64+raspi.img: 12.33 GiB, 13233965056 bytes, 25847588 sectors
Units: sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disklabel type: dos
Disk identifier: 0x354db354

Device                                                                              Boot    Start      End  Sectors       Size Id Type
./ubuntu-2004-adu/src/workspace/ubuntu-20.04.5-preinstalled-server-arm64+raspi.img1 *        2048   526335   524288  268435456  c W95 FAT32 (LBA)
./ubuntu-2004-adu/src/workspace/ubuntu-20.04.5-preinstalled-server-arm64+raspi.img2        526336 11089809 10563474 5408498688 83 Linux
./ubuntu-2004-adu/src/workspace/ubuntu-20.04.5-preinstalled-server-arm64+raspi.img3      11089810 21653283 10563474 5408498688 83 Linux
./ubuntu-2004-adu/src/workspace/ubuntu-20.04.5-preinstalled-server-arm64+raspi.img4      21653284 25847587  4194304 2147483648 83 Linux
```

If the image successfully built and run correctly on the device, CONGRATULATION! Now you're ready for the next steps.

## Install the Device Update Agent and Dependency

> NOTE | This instruction below can be used for integrating the Device Update agent without Azure IoT Edge

The scripts used for integrating the DU Agent into the image can be found in [ubuntu2004-adu/src/modules/ubuntu2004-adu/start_chroot_script](ubuntu2004-adu/src/modules/ubuntu2004-adu/start_chroot_script).

This script perform the following important tasks:

- Add `packages.microsoft.com` to APT repository sources list
- Install the Azure IoT Edge packages
- Install the Delivery Optimization packages
- Install the Device Update for IoT Hub package
- Customize and generate a U-Boot script (boot.scr) to support Dual-Copy update strategy

### Add `packages.microsoft.com` to APT repository sources list

Add following script in `ubuntu2004-adu/src/modules/ubuntu2004-adu/start_chroot_script` to enable packages installation from `packages.microsoft.com`:

```shell
echo "# Add packages.microsoft.com"
adu_setup=/tmp/adu-setup
mkdir -p $adu_setup
curl https://packages.microsoft.com/config/ubuntu/20.04/prod.list > $adu_setup/microsoft-prod.list
cp $adu_setup/microsoft-prod.list /etc/apt/sources.list.d/
curl https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor > $adu_setup/microsoft.gpg
cp $adu_setup/microsoft.gpg /etc/apt/trusted.gpg.d/
apt-get update

echo "# List available versions of Device Update Agent and dependencies "
apt-cache policy deviceupdate-agent deliveryoptimization-agent libdeliveryoptimization libcurl4-openssl-dev libssl1.1
```

### Install the Azure IoT Edge packages

For this tutorial, we will be using the Azure IoT Edge version 1.4.3-1

Add following script in  `ubuntu2004-adu/src/modules/ubuntu2004-adu/start_chroot_script`:

```shell
echo "# Install the moby engine package (required by aziot-edge)"
apt -y install moby-engine

echo_green -e "\n Moby Engine installed successfully\n"

echo "# Install the Azure IoT Edge version 1.4.3-1 package"
apt -y install aziot-edge=1.4.3-1

echo_green -e "\n Azure IoT Edge 1.4.3-1 installed successfully\n"

```

### Install the Delivery Optimization packages

For this tutorial, we will be using the Delivery Optimization 1.0.0

Add following script in  `ubuntu2004-adu/src/modules/ubuntu2004-adu/start_chroot_script`:

```shell
cho "# Install the Delivery Optimization agent version 1.0.0 package"
check_install_pkgs libcurl4-openssl-dev libboost-filesystem1.71.0 libproxy1v5

wget https://github.com/microsoft/do-client/releases/download/v1.0.0/ubuntu2004_arm64-packages.tar -O ubuntu2004_arm64-packages.tar
tar -xvzf ubuntu2004_arm64-packages.tar
rm ubuntu2004_arm64-packages.tar
dpkg -i deliveryoptimization-agent_1.0.0_arm64.deb
rm deliveryoptimization-agent_1.0.0_arm64.deb
dpkg -i libdeliveryoptimization_1.0.0_arm64.deb
rm libdeliveryoptimization_1.0.0_arm64.deb
dpkg -i libdeliveryoptimization-dev_1.0.0_arm64.deb
rm libdeliveryoptimization-dev_1.0.0_arm64.deb

echo_green -e "\nDelivery Optimization 1.0.0 packages installed successfully\n"

```

### Install the Device Update for IoT Hub package

For this tutorial, we will be using the Device Update Agent for IoT Hub version 1.0.0

Add following script in  `ubuntu2004-adu/src/modules/ubuntu2004-adu/start_chroot_script`:

```shell
echo "# Install the Device Update agent version 1.0.0 package"
apt -y install deviceupdate-agent=1.0.0

echo_green -e "\nDevice Update for IoT Hub 1.0.0 package installed successfully\n"

```

Let's build the new image above components integrated. Run `sudo ./ubuntu-2004-adu/src/build_dist`. If everything installed successfully, you should see the follwing output:

```shell
adu-dev@customPiOS-2204-build:~/custompios$ sudo ./ubuntu-2004-adu/src/build_dist

...

Moby Engine installed successfully

...

 Azure IoT Edge 1.4.3-1 installed successfully

...

Delivery Optimization 1.0.0 packages installed successfully

...

Device Update for IoT Hub 1.0.0 package installed successfully

...

BUILD SUCCEEDED!

```

At this point, you should have an image that can be used to connect the device to the Azure IoT Hub and test Azure IoT Edge agent, Azure IoT Identity Service, Deivery Optimization agent and Device Update agent funcionality.

### Using OverlayFS to use Device Update Agent Data from the data partition

By default, the Device Update agent reads configurations from /etc/adu/du-config.json file, writes log info to /var/log/adu folder, and store additional data in /var/lib/adu/ folder. After the image-based update is performed, the device will be rebooted into the new RootFS partition. In this case, some of the data in those folders will not be present in the in the new partition.

To ensure that those data survives the update, we can store any additional data in the `persistent data partition` using OverlayFS.

This can be achieved by using the following steps:

- Create top layer folder in data partition (in `/src/custompios` )
  
    ```shell
    if [ -n "$BUILD_OPTION_CREATE_DATA_PARTITION" ]
    then
        create_partition $BASE_IMG_PATH $BUILD_OPTION_CREATE_DATA_PARTITION_SIZE_BYTES "primary" "$BUILD_OPTION_CREATE_DATA_PARTITION_FSTYPE"
        echo "Create Device Update overlay folders on data partition."
        DATA_PARTITION_MOUNT_PATH="/mnt/adu.o"
        mkdir -p $DATA_PARTITION_MOUNT_PATH
        DATA_LODEV=$(losetup --partscan --show --find $BASE_IMG_PATH )
        mount "${DATA_LODEV}p$DATA_PARTITIION_NUMBER" $DATA_PARTITION_MOUNT_PATH
        mkdir -p $DATA_PARTITION_MOUNT_PATH/etc/adu
        mkdir -p $DATA_PARTITION_MOUNT_PATH/var/lib/adu
        mkdir -p $DATA_PARTITION_MOUNT_PATH/var/log/adu
        mkdir -p $DATA_PARTITION_MOUNT_PATH/work/etc/adu
        mkdir -p $DATA_PARTITION_MOUNT_PATH/work/var/lib/adu
        mkdir -p $DATA_PARTITION_MOUNT_PATH/work/var/log/adu

        echo_green -e "The following folders are succeffully created:"
        tree $DATA_PARTITION_MOUNT_PATH
        umount $DATA_PARTITION_MOUNT_PATH
        losetup -d $DATA_LODEV
    fi
    ```

- Update `fstab` (in `ubuntu2004-adu/src/modules/start_chroot_script`)
  
    ```shell
    # Update fstab data
    echo "/dev/mmcblk0p4   /adu.o    ext4    defaults,sync   0   0" >> /etc/fstab
    echo "overlay /etc/adu overlay noauto,x-systemd.automount,lowerdir=/etc/adu,upperdir=/adu.o/etc/adu,workdir=/adu.o/work/etc/adu 0 0" >> /etc/fstab
    echo "overlay /var/lib/adu overlay noauto,x-systemd.automount,lowerdir=/var/lib/adu,upperdir=/adu.o/var/lib/adu,workdir=/adu.o/work/var/lib/adu 0 0" >> /etc/fstab
    echo "overlay /var/log/adu overlay noauto,x-systemd.automount,lowerdir=/var/log/adu,upperdir=/adu.o/var/log/adu,workdir=/adu.o/work/var/log/adu 0 0" >> /etc/fstab

    ```

### Validate the Device Update Agent functionality

There are 2 type of the IoT Hub twin identities that can be used to connect your device to the Device Update for IoT Hub service; **Device Twin identity** and **Module Twin identity** respectively.

> NOTE | Lean more about [Device Update agent provisioning](https://learn.microsoft.com/azure/iot-hub-device-update/device-update-agent-provisioning) using [Device Twin](https://learn.microsoft.com/azure/iot-hub/iot-hub-devguide-device-twins) or [Module Twin](https://learn.microsoft.com/azure/iot-hub/iot-hub-devguide-module-twins) identity [here](https://learn.microsoft.com/azure/iot-hub/iot-hub-devguide)

To connect your device to the Device Update for IoT Hub service, follow steps below:

**Prerequisites**

- IoT Hub with Device Update for IoT Hub resource (Learn more [here](https://learn.microsoft.com/azure/iot-hub-device-update/create-device-update-account?tabs=portal))
- An IoT Hub Device Twin identity (Learn more [here](https://learn.microsoft.com/azure/iot-edge/how-to-create-iot-edge-device?view=iotedge-2020-11))

Once you obtain your device twin identity, you can provision the IoT Edge agent on the device by running `sudo iotedge config mp --connection-string '...'`

Next, provision the Device Update agent using the Azure Identity Service by simply update follwing properties in `/etc/adu/du-config.json` file:

| Property Path | Value | Description |
|---|---|---|
| /manufacturer | "contoso" | The device manufacturer. This should be the actual device manufacturer company name |
| /model | "raspberri-pi" | The device model. This should be the actual device production model |
| /agents[0]/name | "main" | The agent process name. Default value is "main" |
| /agents[0]/ConnectionType | "AIS" | Acquire connection data from the Azure Identity Service |
| /agents[0]/ConnectionData | "" | Only required for ConnectionType="string" |
| /agents[0]/manufacturer | "contoso" | The device manufacturer. This should be the actual device manufacturer company name |
| /agents[0]//model | "raspberri-pi" | The device model. This should be the actual 
The `etc/adu/du-config.json` should look similar to this:

```json
{
    "schemaVersion": "1.1",
    "aduShellTrustedUsers": [
        "adu",
        "do",
    ],
    "manufacturer": "contoso",
    "model": "respberry-pi",
    "agents": [
        {
            "name": "main",
            "runas": "adu",
            "connectionSource": {
                "connectionType": "AIS",
                "connectionData": ""
            },
            "manufacturer": "contoso",
            "model": "raspberry-pi"
        }
   ]
}
```

Run `sudo systemctl restart deviceupdate-agent` to restart the Device Update agent daemon

Next, go to your IoT Hub web portal and inspect the **Module Twin** data of your device. If the Device Update agent succussfully connected to the IoT Hub, you should see that the device's `reported` properties contains information about the device that you specified in the `/etc/adu/du-config.json` file.

For example:

```json
"reported": {
            "deviceInformation": {
                "__t": "c",
                "manufacturer": "contoso",
                "model": "raspberry-pi",
                "osName": "Ubuntu",
                "swVersion": "20.04.3 LTS ...",
                ...
            },
            "deviceUpdate": {
                "__t": "c",
                "agent": {
                    "deviceProperties": {
                        "manufacturer": "contoso",
                        "model": "raspberry-pi",
                        "contractModelId": "dtmi:azure:iot:deviceUpdateContractModel;2",
                        "aduVer": "DU;agent/1.0.0",
                        "doVer": "..."
                    },
                    "compatPropertyNames": "manufacturer,model",
                    "lastInstallResult": {
                    },
                    "state": 0,
                    "workflow": {
                    },
                    "installedUpdateId": ""
                },
                ...
            },
            ...
            "$version": ...
        }
```

Now, you are ready to deploy some update to your device.

### Customize and generate a U-Boot script (boot.scr) to support Dual-Copy update strategy

By default, the U-Boot script that pre-installed in Ubuntu 20.04 for Raspberry Pi image doesn't support Dual-Copy RootFS updates. We need to be able to tell the U-Boot which parition is the **active RootFS partition**.



    ```sh
    echo "# Generate the boot.scr file"
    pushd /filesystem/boot
    mkimage -A arm -T script -O linux -d boot.cmd.in boot.scr
    cp boot.scr /filesystem/boot/firmware
    popd
    ```
    See modified code in [boot.cmd.in](./ubuntu2004-adu/src/modules/ubuntu2004-adu/filesystem/boot/boot.cmd.in).  

    The customized **boot.scr** file will be located in `/boot/firmware` directory and will be invoked at boot time.
    In a nut-shell, we add additional code to determine which Root FS partition is an active partition.


## How To Test The Image
The test update can be generated and imported to the Device Update Service by follow this [instruction](./example-ubuntu-a-b-update/README.md).
