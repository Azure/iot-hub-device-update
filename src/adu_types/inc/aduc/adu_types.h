/**
 * @file adu_type.h
 * @brief Defines common data types used throughout ADU project.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 */
#ifndef ADUC_ADU_TYPES_H
#define ADUC_ADU_TYPES_H

#include <aduc/c_utils.h>
#include <aduc/logging.h>
#include <stdbool.h>

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

} ADUC_AuthType;

/**
 * @brief IoT Hub connection information.
 */
typedef struct tagConnectionInfo
{
    ADUC_AuthType
        authType; /**< Indicates certificate or shared access signature token authentication for connectionString */
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
void ADUC_ConnectionInfoDeAlloc(ADUC_ConnectionInfo* info);

/**
 * @brief Scans the connection string and returns the connection type related for the string
 * @param connectionString the connection string to scan
 * @returns the connection type for @p connectionString
 */
ADUC_ConnType GetConnTypeFromConnectionString(const char* connectionString);

EXTERN_C_END

#endif // ADUC_ADU_TYPES_H