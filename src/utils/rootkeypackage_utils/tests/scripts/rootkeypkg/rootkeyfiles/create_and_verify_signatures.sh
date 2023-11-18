#!/bin/bash

set -e

# reconstitute the self-signed test private keys
base64 --decode ./testroot1.base64.txt > ./testroot1.pem
base64 --decode ./testroot2.base64.txt > ./testroot2.pem

# get the public keys
openssl rsa -in testroot1.pem -pubout -out pub1.pem
openssl rsa -in testroot2.pem -pubout -out pub2.pem

# write the raw binary signatures
openssl dgst -sign ./testroot1.pem -sha256 -binary ./rootkey.data.json > ./rootkey.data.json.testroot1.binary.sig
openssl dgst -sign ./testroot2.pem -sha256 -binary ./rootkey.data.json > ./rootkey.data.json.testroot2.binary.sig

# write the Base64 URL encoded signatures
openssl dgst -sign ./testroot1.pem -sha256 -binary ./rootkey.data.json | openssl base64 | tr -- '+/=' '-_ ' > ./rootkey.data.json.testroot1.base64url.sig
openssl dgst -sign ./testroot2.pem -sha256 -binary ./rootkey.data.json | openssl base64 | tr -- '+/=' '-_ ' > ./rootkey.data.json.testroot2.base64url.sig

# Verify the raw binary signatures against the data that was signed (rootkey.data.json)
openssl dgst -verify ./pub1.pem -sha256 -binary -signature ./rootkey.data.json.testroot1.binary.sig ./rootkey.data.json
openssl dgst -verify ./pub2.pem -sha256 -binary -signature ./rootkey.data.json.testroot2.binary.sig ./rootkey.data.json
