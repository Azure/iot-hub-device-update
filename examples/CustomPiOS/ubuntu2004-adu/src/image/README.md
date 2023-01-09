# Instruction

## For Rasbian OS

Place zipped Rasbian image here.
If not otherwise specified, the build script will always use the most
recent zip file matching the file name pattern "*-raspbian.zip" located
here.

## For Ubuntu OS

Download 
[ubuntu-20.04.5-preinstalled-server-arm64+raspi.img.xz](http://cdimage.ubuntu.com/releases/20.04/release/ubuntu-20.04.5-preinstalled-server-arm64+raspi.img.xz) and place it here.

> NOTE | must set `BOOT_ZIP_IMG` in [config](../config) file
>
> For example:
> export BASE_ZIP_IMG=ubuntu-20.04.5-preinstalled-server-arm64+raspi.img.xz

