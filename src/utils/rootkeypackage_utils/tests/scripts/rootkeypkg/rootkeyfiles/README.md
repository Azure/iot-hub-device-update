# Test Data for rootkeypackage_utils

## Root Key Package

The root key package is rootkey.json file.

## create_and_verify_signatures.sh

create_and_verify_signatures.sh will:

- Generate private and public keys,
- Create rootkey.data.json.testrootX.(base64 | binary).sig,
- and verify the signatures against rootkey.data.json data that was signed.

## Key pairs

Regenerate the key by doing:

```bash
openssl genrsa 2048 | openssl base64 > testrootX.base64.txt
```

## Modulus and Exponent

Get the RSA modulus and exponent by doing:

```bash
openssl rsa -pubin -inform PEM -text -noout < pubX.pem
```
