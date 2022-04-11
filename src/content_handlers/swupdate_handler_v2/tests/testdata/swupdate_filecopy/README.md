# SWUpdate Test 1

## Build .SWU file

To produce an image, a script like this can be used:

Example of sw-description that contains preinstall script

```txt
CONTAINER_VER="1.0"
PRODUCT_NAME="du-agent-swupdate-filecopy-test-1"
FILES="sw-description preinst.sh mock-update-for-file-copy-test-1.txt"
for i in $FILES;do
        echo $i;done | cpio -ov -H crc >  ${PRODUCT_NAME}_${CONTAINER_VER}.swu
```
