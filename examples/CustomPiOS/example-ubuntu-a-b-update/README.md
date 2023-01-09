# Testing The Scripts

## How To Test demo-a-b-rootfs-script.sh

```sh
wf=$/tmp
sudo mkdir -p $wf
logfile=$wf/1.log
outfile=$wf/1.out
resfile=$wf/1.res.json
swverfile=/etc/du-image-version
incri=ubuntu-20.04-adu-1.0.6

# Test IsInstalled task
sudo ./demo-a-b-rootfs-script.sh --work-folder $wf --log-file $logfile --output-file $outfile --result-file $resfile --software-version-file $swverfile --installed-criteria $incri --action-is-installed

cat $resfile

sudo ./demo-a-b-rootfs-script.sh --work-folder $wf --log-file $logfile --output-file $outfile --result-file $resfile --software-version-file $swverfile --installed-criteria "$incri" --action-download

cat $resfile

sudo ./demo-a-b-rootfs-script.sh --work-folder $wf --log-file $logfile --output-file $outfile --result-file $resfile --software-version-file $swverfile --installed-criteria "$incri" --action-install

cat $resfile


```
