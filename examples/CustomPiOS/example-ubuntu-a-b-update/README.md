# Testing The Scripts

## How To Test demo-a-b-rootfs-script.sh 

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
