#!/bin/bash
echo Generating signature for $1
openssl dgst -sha256 -sign /mnt/c/adu/diffgen-tool.Release.x64-linux/swupdate-priv.pem -out $1.sig $1
