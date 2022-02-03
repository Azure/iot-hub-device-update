/**
 * @file adu_type.h
 * @brief Defines common types used throughout Device Update agent components.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_ADU_TYPES_H
#define ADUC_ADU_TYPES_H

#include <stdbool.h> // for bool
#include <stddef.h> // for size_T

#include "aduc/c_utils.h"
#include "aduc/logging.h"

EXTERN_C_BEGIN
/**
 * @brief ADU Client launch arguments.
 */
typedef struct tagADUC_LaunchArguments
{
    int argc; /**< Size of argv */
    char** argv; /**< Command-line arguments */
    ADUC_LOG_SEVERITY logLevel; /**< Log level */
    char* connectionString; /**< Device connection string from command-line. */
    bool iotHubTracingEnabled; /**< Whether to enable logging from IoT Hub SDK */
    bool showVersion; /**< Show an agent version */
    bool healthCheckOnly; /**< Only check agent health. Doesn't process any data or messages from services. */
    char* contentHandlerFilePath; /**< A full path of an update content handler to be registered */
    char* componentEnumeratorFilePath; /**< A full path of a component enumerator to be registered */
    char* contentDownloaderFilePath; /**< A full path of a content downloader to be registered */
    char* updateType;
} ADUC_LaunchArguments;

typedef enum tagADUC_ConnType
{
    ADUC_ConnType_NotSet = 0,
    ADUC_ConnType_Device = 1,
    ADUC_ConnType_Module = 2,
} ADUC_ConnType;

typedef enum tagADUC_AuthType
{
    ADUC_AuthType_NotSet = 0,
    ADUC_AuthType_SASToken = 1,
    ADUC_AuthType_SASCert = 2,
    ADUC_AuthType_NestedEdgeCert = 3,
} ADUC_AuthType;

/**
 * @brief IoT Hub connection information.
 */
typedef struct tagConnectionInfo
{
    ADUC_AuthType authType; /**< Indicates the authentication type for connectionString */
    ADUC_ConnType connType; /**< Indicates whether this connection string is module-id or device-id based */
    char* connectionString; /**< Device or Module connection string. */
    char* certificateString; /**< x509 certificate in PEM format for the IoTHubClient to be used for authentication*/
    char* opensslEngine; /**< identifier for the OpenSSL Engine used for the certificate in certificateString*/
    char* opensslPrivateKey; /**< x509 private key in PEM format for the IoTHubClient to be used for authentication */
} ADUC_ConnectionInfo;

/**
 * @brief DeAllocates the ADUC_ConnectionInfo object
 * @param info the ADUC_ConnectionInfo object to be de-allocated
 */
void ADUC_ConnectionInfo_DeAlloc(ADUC_ConnectionInfo* info);

/**
 * @brief Returns the string associated with @p connType
 * @param connType ADUC_ConnType to be stringified
 * @returns if the ADUC_ConnType exists then the string version of the value is returned, "" otherwise
 */
const char* ADUC_ConnType_ToString(const ADUC_ConnType connType);

EXTERN_C_END

#endif // ADUC_ADU_TYPES_H
