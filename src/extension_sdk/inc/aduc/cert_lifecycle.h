/**
 * @file cert_lifecycle.h
 * @brief Certificate management for mTLS connections.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_CERT_LIFECYCLE_H
#define ADUC_CERT_LIFECYCLE_H

#include "aduc/extension_types.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Certificate information.
 */
typedef struct ADUC_CertInfo
{
    char subjectCN[256];
    char issuerCN[256];
    char serialNumber[64];
    uint64_t notBefore;       // Unix timestamp
    uint64_t notAfter;        // Unix timestamp
    bool isExpired;
    uint32_t daysUntilExpiry;
} ADUC_CertInfo;

/**
 * @brief Load and inspect a certificate.
 */
ADUC_Result2 ADUC_Cert_GetInfo(const char* certPath, ADUC_CertInfo* outInfo);

/**
 * @brief Check if certificate needs renewal (within threshold days).
 */
bool ADUC_Cert_NeedsRenewal(const ADUC_CertInfo* info, uint32_t thresholdDays);

/**
 * @brief Validate certificate chain.
 */
ADUC_Result2 ADUC_Cert_ValidateChain(const char* certPath, const char* caPath);

#ifdef __cplusplus
}
#endif

#endif // ADUC_CERT_LIFECYCLE_H
