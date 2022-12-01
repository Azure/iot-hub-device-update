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

typedef enum tagADUC_ExtensionRegistrationType
{
    ExtensionRegistrationType_None,
    ExtensionRegistrationType_UpdateContentHandler,
    ExtensionRegistrationType_ContentDownloadHandler,
    ExtensionRegistrationType_ComponentEnumerator,
    ExtensionRegistrationType_DownloadHandler,
} ADUC_ExtensionRegistrationType;

/**
 * @brief ADU Client launch arguments.
 */
typedef struct tagADUC_LaunchArguments
{
    char** argv; /**< Command-line arguments */
    char* ipcCommand; /**< an inter-process command to be insert into a command queue. */
    char* connectionString; /**< Device connection string from command-line. */
    char* contentDownloaderFilePath; /**< A full path of a content downloader to be registered. */
    char* extensionFilePath; /**< The path to the extension shared library file. */
    char* extensionId; /**< The extension id for an extension registration type like
                            downloadHandlerId for download handlers and updateType for content handlers. */
    int argc; /**< Size of argv */
    ADUC_LOG_SEVERITY logLevel; /**< Log level */
    ADUC_ExtensionRegistrationType extensionRegistrationType; /**< The type of extension being registered. */
    bool iotHubTracingEnabled; /**< Whether to enable logging from IoT Hub SDK. */
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

/**
 * @brief Struct containing information about the IoT Hub device/model client PnP property update notification.
 *
 */
typedef struct tagADUC_PnPComponentClient_PropertyUpdate_Context
{
    bool clientInitiated; /** Indicates that the property update notification was caused by a client request.
                               For example, when the agent call IoTHub_DeviceClient_LL_GetTwinAsync API.
                               Note: this value should be set to null when calling ClientHandle_SetClientTwinCallback. */
    bool forceUpdate; /** In indicates whether the force process the update. */
} ADUC_PnPComponentClient_PropertyUpdate_Context;

EXTERN_C_END

#endif // ADUC_ADU_TYPES_H
