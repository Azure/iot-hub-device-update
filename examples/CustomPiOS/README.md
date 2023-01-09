# Create Ubuntu 20.04 with the IoT Hub Device Update for Raspberry Pi Hardware

## Goals
- Integrate the Device Update agent using DEB package from `packages.microsoft.com`
- Support image-based update using A/B update strategy.

## Create Additional Partitions

### Secondary Root FS Partition
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

### Add Helper Function For Partition

- Add `copy_partition` and `new_partition` to `src/common.sh`
    ```sh
    # Usage copy_partition $base_img_path $source_partition_number $new_partition_number 'primary' 'ext4'
    function copy_partition() {
      image=$1
      partition=$2
      new_partition=$3
      new_partition_type=$4
      new_partition_fstype=$5
      buffer=0
    
      sector_size=512
      one_mi=$((1000 * 1000))
      one_mb=$((1024 * 1024))
    
      echo "Copying partition $partition on $image to partition $new_partition"
      partitioninfo=$(sfdisk -d $image | grep "$image$partition")
      
      start=$(echo $partitioninfo | awk '{print $4-0}')
      e2fsize_blocks=$(echo $partitioninfo | awk '{print $6-0}')
      offset=$(($start * $sector_size))
    
      detach_all_loopback $image
      test_for_image $image
      LODEV=$(losetup -f --show -o $offset $image)
      trap 'losetup -d $LODEV' EXIT
    
      e2fsck -fy $LODEV
      e2fblocksize=$(tune2fs -l $LODEV | grep -i "block size" | awk -F: '{print $2-0}')
      e2fminsize=$(resize2fs -P $LODEV 2>/dev/null | grep -i "minimum size" | awk -F: '{print $2-0}')
    
      e2fminsize_bytes=$(($e2fminsize * $e2fblocksize))
    
      e2ftarget_bytes=$(($buffer * $one_mb + $e2fminsize_bytes))
      e2fsize_bytes=$(($e2fsize_blocks * $sector_size))
    
      e2fminsize_mb=$(($e2fminsize_bytes / $one_mb))
      e2fminsize_blocks=$(($e2fminsize_bytes / $sector_size + 1))
      e2ftarget_mb=$(($e2ftarget_bytes / $one_mb))
      e2ftarget_blocks=$(($e2ftarget_bytes / $sector_size + 1))
      e2fsize_mb=$(($e2fsize_bytes / $one_mb))
      
      size_offset_mb=$(($e2fsize_mb - $e2ftarget_mb))
      
      losetup -d $LODEV
    
      echo "Source partition actual size is $e2fsize_mb MB ($e2fsize_blocks blocks), Minimum size is $e2fminsize_mb MB ($e2fminsize file system blocks, $e2fminsize_blocks blocks)"
    
      # Expand the image file
      echo "Enlarge the image file..."
      dd if=/dev/zero bs=$one_mb count=$(($e2fsize_mb + 1)) >> $image
    
      echo "Re-mount the image..."
      detach_all_loopback $image
      test_for_image $image
      LODEV=$(losetup --partscan --show --find $image)
    
      echo "Create new partition "
      # NOTE: Size belows are in Mi unit.
      start_mi=$((((($start + $e2fsize_blocks) * $sector_size) / $one_mi) + 1))
      end_mi=$(($start_mi + ((($e2fsize_blocks * $sector_size) / $one_mi) + 1))) 
      parted $LODEV --script mkpart $new_partition_type $new_partition_fstype $start_mi $end_mi
      part_text=p$new_partition
      mkfs.ext4 "$LODEV$part_text"
    
      detach_all_loopback $image
      test_for_image $image
      LODEV=$(losetup --partscan --show --find $image)
      trap 'losetup -d $LODEV' EXIT
    }
    
    # Usage new_partition $base_img_path $new_partition_number $new_partition_size_mb 'primary' 'ext4'
    function new_partition() {
      image=$1
      new_partition_number=$2
      new_partition_size_mb=$3
      new_partition_type=$4
      new_partition_fstype=$5
    
      detach_all_loopback $image
      test_for_image $image
    
      # Expand the image file
      echo "Adding $new_partition_size_mb (MB) to the image file..."
      dd if=/dev/zero bs=$one_mb count=$new_partition_size_mb >> $image
    
      test_for_image $image
      LODEV=$(losetup --partscan --show --find $image)
    
      echo "Current free spaces:"
      parted $LODEV --script print free
    
      free_space_txt=$(parted $LODEV --script print free | grep 'Free.Space' | tail -1)
      start=$(echo "$free_space_txt" | awk '{print $1-0}')
      end=$(echo "$free_space_txt" | awk '{print $2-0}')
      size=$(echo "$free_space_txt" | awk '{print $3-0}')
    
      echo "Crating new partition at block $start - $end"
      parted $LODEV --script mkpart $new_partition_type $new_partition_fstype $start $end
      part_text=p$new_partition_number
      mkfs.ext4 "$LODEV$part_text"
    
      detach_all_loopback $image
      test_for_image $image
      LODEV=$(losetup -f --show -o $offset $image)
      trap 'losetup -d $LODEV' EXIT
    }
    
    ```
- Modify `src/custompios` script to call the new helper function to create additional partitions. Add following scripts at the end of the build process.
    ```sh

      echo "Creating additional Root FS partition..."
      # Partition number
      new_root_partition=$(($BASE_ROOT_PARTITION + 1))
      # Create new partition the same size as ###.img2, but don't copy any data.
      copy_partition $BASE_IMG_PATH $BASE_ROOT_PARTITION $new_root_partition 'primary' 'ext4'
    
      echo "Creating additional data partition..."
      # Partition number
      new_persistent_partition=$(($BASE_ROOT_PARTITION + 2))
      # Size in MiB
      new_persistent_partition_size_mb=1000
      new_partition $BASE_IMG_PATH $new_persistent_partition $new_persistent_partition_size_mb 'primary' 'ext4'
    
    ```

## Install the Device Update Agent and Dependency

> NOTE | This instruction below can be used for integrating the Device Update agent without Azure IoT Edge

The scripts used for integrating the DU Agent into the image can be found in [ubuntu2004-adu/src/modules/ubuntu2004-adu/start_chroot_script](ubuntu2004-adu/src/modules/ubuntu2004-adu/start_chroot_script).

This script perform the following important tasks:

- Generate a customized U-Boot script (`boot.scr`) to support Root FS selection  
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

- Install Device Update Agent package and its dependencies
    ```sh
    echo '# Add packages.microsoft.com'
    adu_setup=/tmp/adu-setup
    mkdir -p $adu_setup
    curl https://packages.microsoft.com/config/ubuntu/20.04/prod.list > $adu_setup/microsoft-prod.list
    cp $adu_setup/microsoft-prod.list /etc/apt/sources.list.d/
    curl https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor > $adu_setup/microsoft.gpg
    cp $adu_setup/microsoft.gpg /etc/apt/trusted.gpg.d/
    apt-get update
        
    echo "# Install Device Update Agent and Dependencies "
    
    wget https://github.com/microsoft/do-client/releases/download/v1.0.0/ubuntu2004_arm64-packages.tar -O ubuntu2004_arm64-packages.tar
    tar -xvzf ubuntu2004_arm64-packages.tar
    rm ubuntu2004_arm64-packages.tar
    dpkg -i deliveryoptimization-agent_1.0.0_arm64.deb
    rm deliveryoptimization-agent_1.0.0_arm64.deb
    dpkg -i libdeliveryoptimization_1.0.0_arm64.deb
    rm libdeliveryoptimization_1.0.0_arm64.deb
    dpkg -i libdeliveryoptimization-dev_1.0.0_arm64.deb
    rm libdeliveryoptimization-dev_1.0.0_arm64.deb
    
    apt install -y deviceupdate-agent
    ```

