# Testing The Scripts

## How To Test demo-a-b-rootfs-script.sh on Ubuntu Device or VM 

To manually test the script on the customized device (must have /dev/mmcblk0p2 and /dev/mmcblk0p3 as Root FS), set following environment variables, then try the [test commands]() below.

```sh
cd ~
wf=~/tmp
sudo mkdir -p $wf
logfile=$wf/demo-rootfs-installer.log
outfile=$wf/demo-ab-rootfs-update.out
resfile=$wf/demo-ab-rootfs-update.result.json
swverfile=$wf/demo-du-image-version
inst_criteria=ubuntu-20.04-adu-1.0.6
blfolder=$wf/firmware
sudo mkdir -p $blfolder
```

Below are environment variables that resembled the device built from our CustomPiOS project configuration

```sh
cd ~
wf=~/tmp
sudo mkdir -p $wf
logfile=$wf/demo-rootfs-installer.log
outfile=$wf/demo-ab-rootfs-update.out
resfile=$wf/demo-ab-rootfs-update.result.json
swverfile=/etc/du-image-version
inst_criteria=ubuntu-20.04-adu-1.0.6
blfolder=/boot/firmware
sudo mkdir -p $blfolder
```

## Test Commands

### Test --action-is-installed

```sh
sudo ./demo-a-b-rootfs-script.sh --work-folder $wf --log-file $logfile --output-file $outfile --result-file $resfile --software-version-file $swverfile --installed-criteria "$inst_criteria" --boot-loader-folder "$blfolder" --action-is-installed

cat $resfile
```

### Test --action-download

```sh
sudo ./demo-a-b-rootfs-script.sh --work-folder $wf --log-file $logfile --output-file $outfile --result-file $resfile --software-version-file $swverfile --installed-criteria "$inst_criteria" --boot-loader-folder "$blfolder" --action-download

cat $resfile
```

### Test --action-install (Performs actual file copy, to speed up the process)

```sh
sudo ./demo-a-b-rootfs-script.sh --work-folder $wf --log-file $logfile --output-file $outfile --result-file $resfile --software-version-file $swverfile --installed-criteria "$inst_criteria" --boot-loader-folder "$blfolder" --action-install 

cat $resfile
```

### Test --action-install (Skip file copy)

```sh
sudo ./demo-a-b-rootfs-script.sh --work-folder $wf --log-file $logfile --output-file $outfile --result-file $resfile --software-version-file $swverfile --installed-criteria "$inst_criteria" --boot-loader-folder "$blfolder" --action-install --debug-install-no-file-copy

cat $resfile
```

### Test --action-apply (no device restart)

```sh
sudo ./demo-a-b-rootfs-script.sh --work-folder $wf --log-file $logfile --output-file $outfile --result-file $resfile --software-version-file $swverfile --installed-criteria "$inst_criteria" --boot-loader-folder "$blfolder" --action-apply

cat $resfile
```

### Test --action-apply (Restart device after apply succeeded)

```sh
sudo ./demo-a-b-rootfs-script.sh --work-folder $wf --log-file $logfile --output-file $outfile --result-file $resfile --software-version-file $swverfile --installed-criteria "$inst_criteria"  --boot-loader-folder "$blfolder" --action-is-installed

cat $resfile
```

## How To Generate The Device Update Import Manifest and Payloads

To generate the import manifest and the update payloads that can be imported into the Device Update Service, follow this instruction:

- Open a PowerShell console
- Create a working folder
    ```powershell
    mkdir ~/myupdate
    ```
- Change to the working folder
    ```powershell
    pushd ~/myupdate
    ```
- Copy [AduUpdate.psm1](../../../tools/AduCmdlets/AduUpdate.psm1) to the working folder, then call `Import-Module`
    ```powershell
    cp <filelocation>/create-demo-a-b-rootfs-update-import-manifest.ps1 .
    cp -s <fileloction>/payloads ./
    ```
- Copy [create-demo-a-b-rootfs-update-import-manifest.ps1](./create-demo-a-b-rootfs-update-import-manifest.ps1) and [payloads](./payloads/) folder into the working folder
    ```powershell
    cp <filelocation>/create-demo-a-b-rootfs-update-import-manifest.ps1 .
    cp -s <fileloction>/payloads ./
    ```
- Generate the import manifest. You can change the update version number as needed. The period (.) at the end indicates that the output will be placed in the current folder.
    ```powershell
    ./create-demo-a-b-rootfs-update-import-manifest.ps1 -UpdateVersion "1.0.1" .
    ```
You should see the output like this:
```sh
#  
# Generating an import manifest
#
# Update version : 1.0.1
#
# Payloads :
#    \home\me\myupdate\payloads\demo-a-b-rootfs-script.sh
#
#    Output directory: fabrikam.rpi-ubuntu-2004-adu-update.1.0.1
#
#    Import manifest: .\fabrikam.rpi-ubuntu-2004-adu-update.1.0.1\fabrikam.rpi-ubuntu-2004-adu-update.1.0.1.importmanifest.json
#    Saved payload(s) to: .\fabrikam.rpi-ubuntu-2004-adu-update.1.0.1
#
```
