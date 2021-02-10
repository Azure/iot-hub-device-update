/**
 * @file eis_utils.h
 * @brief Header file for the Edge Identity Service (EIS) Utility
 *
 * @copyright Copyright (c) 2019, Microsoft Corporation.
 */
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

typedef enum tagEISConnType
{
    EISConnType_NotSet = 0,
    EISConnType_Device = 1,
    EISConnType_Module = 2,

} EISConnType;

typedef enum tagEISAuthType
{
    EISAuthType_NotSet = 0,
    EISAuthType_SASToken = 1,
    EISAuthType_SASCert = 2,

} EISAuthType;

/**
 * @brief Struct that encapsulates the information used to provision a connection with EIS
 * @details Please note that this struct should be freed using EISProvisioningInfoDeAlloc()
 */
typedef struct tagEISProvisioningInfo
{
    char* connectionString; /**< String for creating a connection to the IotHub */
    EISConnType connType; /**< Type of connection to be made, can be EISConnType_Device or EISConnType_Module*/
    EISAuthType
        authType; /**< The authentication method used with this connection string, can be EIsAuthType_SAS or EISAuthType_x509*/
    char* certKeyHandle; /**< The private key handle associated with the certificate used to authenticate the service*/
    char*
        certificateString; /**< The certificate string in pem format, Used for creating a connection to an IotHub when using a x509*/
} EISProvisioningInfo;

/**
 * @brief Helper function that de allocates the members of the EISProvisioningInfo struct
 * @param info the struct who's members shall be de-allocated
 */
void EISProvisioningInfoDeAlloc(EISProvisioningInfo* info);

/**
 * @brief Creates a connection string using the provisioned data within EIS
 * @details Calls into the EIS Identity and Keyservice to create a SharedAccessSignature which is then used 
 * to create the connection string, Caller is required to call free() to deallocate the connection string
 * @param[in] expirySecsSinceEpoch the expiration time in seconds since the epoch for the token in the connection string
 * @param[in] timeoutMS the timeoutMS in milliseconds for each call to EIS
 * @param[out] provisioningInfo the pointer to the struct which will be initialized with the information for creating a connection to IotHub using the EIS supported provisioning information
 * @returns on success a null terminated connection string, otherwise NULL
 */
EISUtilityResult RequestConnectionStringFromEISWithExpiry(
    const time_t expirySecsSinceEpoch, uint32_t timeoutMS, EISProvisioningInfo* provisioningInfo);

EXTERN_C_END

#endif