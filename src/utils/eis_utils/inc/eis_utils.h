/**
 * @file eis_utils.h
 * @brief Header file for the Edge Identity Service (EIS) Utility
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <aduc/adu_types.h>
#include <aduc/c_utils.h>
#include <eis_err.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#ifndef EIS_UTILS_H
#    define EIS_UTILS_H

EXTERN_C_BEGIN

/**
 * @brief OpenSSL Key Engine ID to be set within the IotHub connection if using EIS to provision with a cert
 */
#    define EIS_OPENSSL_KEY_ENGINE_ID "aziot_keys"

/**
 * @brief Creates a connection string using the provisioned data within EIS
 * @details Calls into the EIS Identity and Keyservice to create a SharedAccessSignature which is then used
 * to create the connection string, Caller is required to call ADUC_ConnectionInfo_DeAlloc() to deallocate the ADUC_ConnectionInfo struct
 * @param[in] expirySecsSinceEpoch the expiration time in seconds since the epoch for the token in the connection string
 * @param[in] timeoutMS the timeoutMS in milliseconds for each call to EIS
 * @param[out] provisioningInfo pointer to the struct which will be initialized with the information for creating a connection to IotHub
 * @returns on success a null terminated connection string, otherwise NULL
 */
EISUtilityResult RequestConnectionStringFromEISWithExpiry(
    const time_t expirySecsSinceEpoch, uint32_t timeoutMS, ADUC_ConnectionInfo* provisioningInfo);

EXTERN_C_END

#endif
