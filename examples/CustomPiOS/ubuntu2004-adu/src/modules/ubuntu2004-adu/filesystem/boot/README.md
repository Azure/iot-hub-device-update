# The `boot` Folder Content

What you put in this folder will go to the boot partition, you can add config files that would be accessible to windows users.

## How To Build boot.scr

mkimage -A arm -T script -O linux -d boot.cmd.in boot.scr
