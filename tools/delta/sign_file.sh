#!/bin/bash
echo Generating signature for $1
openssl dgst -sha256 -sign [path to your .pem file] -out $1.sig $1
