/**
 * @file adu_direct_mtls.h
 * @brief Client mTLS configuration helper for ADU Direct communication.
 *
 * Provides functions to load, validate, and apply mTLS configuration
 * to CURL handles for secure communication with ADU services.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADU_DIRECT_MTLS_H
#define ADU_DIRECT_MTLS_H

#include "aduc/extension_types.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief mTLS configuration for ADU Direct communication.
 */
typedef struct ADUC_MtlsConfig
{
    char certPath[256];
    char keyPath[256];
    char caPath[256];
    bool verifyPeer;
} ADUC_MtlsConfig;

/**
 * @brief Apply mTLS configuration to a CURL handle.
 *
 * Sets CURLOPT_SSLCERT, CURLOPT_SSLKEY, CURLOPT_CAINFO, and
 * CURLOPT_SSL_VERIFYPEER on the provided CURL handle.
 *
 * @param curlHandle  Pointer to an initialized CURL* handle.
 * @param config      mTLS configuration to apply.
 * @return ADUC_Result2 Success (code=0) or failure.
 */
ADUC_Result2 ADUC_Mtls_ApplyToCurl(void* curlHandle, const ADUC_MtlsConfig* config);

/**
 * @brief Load mTLS configuration from a TOML config section.
 *
 * Reads cert_path, key_path, ca_path, and verify_peer from the
 * specified configuration section.
 *
 * @param configSection  TOML section name (e.g., "mtls").
 * @param outConfig      Output configuration struct.
 * @return ADUC_Result2 Success (code=0) or failure.
 */
ADUC_Result2 ADUC_Mtls_LoadConfig(const char* configSection, ADUC_MtlsConfig* outConfig);

/**
 * @brief Validate that cert/key files exist and are readable.
 *
 * Checks access() on certPath, keyPath, and caPath (if non-empty).
 *
 * @param config  mTLS configuration to validate.
 * @return ADUC_Result2 Success (code=0) or failure with details.
 */
ADUC_Result2 ADUC_Mtls_ValidateFiles(const ADUC_MtlsConfig* config);

#ifdef __cplusplus
}
#endif

#endif // ADU_DIRECT_MTLS_H
