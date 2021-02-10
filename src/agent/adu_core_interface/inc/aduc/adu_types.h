/**
 * @file adu_type.h
 * @brief Defines common data types used throughout ADU project.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 */
#ifndef ADUC_ADU_TYPES_H
#define ADUC_ADU_TYPES_H

#include <aduc/logging.h>
#include <stdbool.h>

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

/**
 * @brief IoT Hub connection information.
 */
typedef struct tagConnectionInfo
{
    char* connectionString; /**< Device or Module connection string. */
    char* certificateString; /**< x509 certificate in PEM format for the IoTHubClient to be used for authentication*/
    char* opensslEngine; /**< identifier for the OpenSSL Engine used for the certificate in certificateString*/
    char* opensslPrivateKey; /**< x509 private key in PEM format for the IoTHubClient to be used for authentication */
} ADUC_ConnectionInfo;

#endif // ADUC_ADU_TYPES_H