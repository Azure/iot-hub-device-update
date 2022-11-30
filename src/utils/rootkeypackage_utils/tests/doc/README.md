# Test Data for rootkeypackage_utils

## PEM

testrootkeyX.pem.txt were generated with:

```bash
openssl genrsa -out testrootkeyX.pem.txt 2048
```

Can turn it into .pem by

URLUInt decode it and adding

header:
`-----BEGIN RSA PRIVATE KEY-----`

and footer:
`-----END RSA PRIVATE KEY-----`

## Public

pubX.txt were created with:

```bash
openssl rsa -in testrootX.txt -pubout -out pubX.txt
```

Can turn it into .key pem by

URLUInt decode it and adding

header:
`-----BEGIN PUBLIC KEY-----`

and footer:
`-----END PUBLIC KEY-----`

## SHA256 hash of Public

After URLUInt decode and restore header/footer:

```bash
# der(binary) format
openssl rsa -in pubX.pem -pubin -outform der | openssl dgst -sha256 | openssl base64 > pubX.sha256.txt
```

## Signature Files - testrootX.sig

```bash
openssl dgst -sha256 -sign testrootX.pem -out valid_test_rootkeypackage__protected.json testrootX.sig
```

## Modulus and Exponent

mod_exp_X.txt were produced via:

```bash
openssl rsa -pubin -inform PEM -text -noout < pubX.txt
```
