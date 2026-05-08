/**
 * @file security_logstrings.h
 * @brief Log format strings for security operations.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_SECURITY_LOGSTRINGS_H
#define ADUC_SECURITY_LOGSTRINGS_H

#define ADUC_LOG_SEC_ROOTKEY_LOAD       "Loading root key package from %s"
#define ADUC_LOG_SEC_ROOTKEY_OK         "Root key package validated: %d keys loaded"
#define ADUC_LOG_SEC_ROOTKEY_FAIL       "Root key package validation FAILED: %s"
#define ADUC_LOG_SEC_JWS_VERIFY         "Verifying JWS signature: kid=%s"
#define ADUC_LOG_SEC_JWS_OK             "JWS signature verified successfully"
#define ADUC_LOG_SEC_JWS_FAIL           "JWS signature verification FAILED: %s"
#define ADUC_LOG_SEC_CERT_LOAD          "Loading client certificate: %s"
#define ADUC_LOG_SEC_CERT_EXPIRY        "Certificate expires: %s (days remaining: %d)"
#define ADUC_LOG_SEC_HASH_COMPUTE       "Computing %s hash for file: %s"

#endif // ADUC_SECURITY_LOGSTRINGS_H
