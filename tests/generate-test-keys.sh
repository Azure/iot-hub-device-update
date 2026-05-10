#!/bin/bash
# Generate test ECDSA keypair for ADU extension signing tests.
#
# Usage: ./generate-test-keys.sh [output-dir]
#
# Produces:
#   test-signing-key.pem   — EC private key (prime256v1)
#   test-signing-cert.pem  — Self-signed X.509 certificate (365 days)

set -euo pipefail

OUTPUT_DIR="${1:-.}"

openssl ecparam -genkey -name prime256v1 -out "${OUTPUT_DIR}/test-signing-key.pem"
openssl req -new -x509 \
    -key "${OUTPUT_DIR}/test-signing-key.pem" \
    -out "${OUTPUT_DIR}/test-signing-cert.pem" \
    -days 365 \
    -subj "/CN=ADU Test Signing"

echo "Generated test keypair in ${OUTPUT_DIR}:"
echo "  Private key:  ${OUTPUT_DIR}/test-signing-key.pem"
echo "  Certificate:  ${OUTPUT_DIR}/test-signing-cert.pem"
