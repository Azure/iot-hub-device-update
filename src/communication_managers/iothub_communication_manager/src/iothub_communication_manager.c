/**
 * @file iothub_communication_manager.c
 * @brief Implements the IoT Hub communication manager utility.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/iothub_communication_manager.h"
#include "aduc/adu_types.h"
#include "aduc/client_handle_helper.h"
#include "aduc/config_utils.h"
#include "aduc/connection_string_utils.h" // ConnectionStringUtils_DoesKeyExist
#include "aduc/d2c_messaging.h" // ADUC_D2C_Messaging_Reset_For_Handle_Refresh
#include "aduc/https_proxy_utils.h"
#include "aduc/logging.h"
#include "aduc/retry_utils.h"
#include "aduc/string_c_utils.h" // LoadBufferWithFileContents
#include <azure_c_shared_utility/shared_util_options.h>

#include "eis_utils.h"

#include <iothub.h>
#include <iothub_client_options.h>

#include <assert.h>

#ifdef ADUC_ALLOW_MQTT
#    include <iothubtransportmqtt.h>
#endif

#ifdef ADUC_ALLOW_MQTT_OVER_WEBSOCKETS
#    include <iothubtransportmqtt_websockets.h>
#endif

#include <aducpal/time.h> // ADUCPAL_clock_gettime
#include <aducpal/unistd.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h> // strtol
#include <sys/stat.h>

#include <pthread.h>

/**
 * @brief A pointer to ADUC_ClientHandle data. This must be initialize by the component that creates the IoT Hub connection.
 */
static ADUC_ClientHandle* g_aduc_client_handle_address = NULL;

/**
 * @brief A mutex to protect the IoT Hub connection handle.
 */
static pthread_mutex_t s_client_handle_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief A callback function to be invoked when a device client handler has changed.
 */
static ADUC_COMMUNICATION_MANAGER_CLIENT_HANDLE_UPDATED_CALLBACK g_iothub_client_handle_changed_callback = NULL;

/**
 * @brief A callback function to be invoked when a device client received the twin data.
 */
static IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK g_device_twin_callback = NULL;

/**
 * @brief A boolean indicates whether the IoT Hub client has been initialized.
 */
static bool g_iothub_client_initialized = false;

/**
 * @brief An additional data context used the caller.
 */
static ADUC_PnPComponentClient_PropertyUpdate_Context* g_property_update_context = NULL;

static time_t g_last_authenticated_time = 0; // The last authenticated timestamp (since epoch)
static time_t g_next_authentication_attempt_time = 0; // Time stamp when we should try to authenticate with the hub.
static time_t g_first_unauthenticated_time = 0; // The first unauthenticated timestamp (since epoch)
static time_t g_last_authentication_attempt_time = 0; // The last authentication attempt timestamp (since epoch)
static time_t g_last_connection_status_callback_time =
    0; // The last time the connection callback was called (since epoch)
static unsigned int g_authentication_retries = 0; // The total authentication retries count.

// Engine type for an OpenSSL Engine
static const OPTION_OPENSSL_KEY_TYPE x509_key_from_engine = KEY_TYPE_ENGINE;

/**
 * @brief The Device Twin Model Identifier.
 * This model must contain 'azureDeviceUpdateAgent' and 'deviceInformation' sub-components.
 *
 * Customers should change this ID to match their device model ID.
 */
static const char g_aduModelId[] = "dtmi:azure:iot:deviceUpdateModel;3";

/**
 * @brief Current connection status.
 */
IOTHUB_CLIENT_CONNECTION_STATUS g_connection_status = IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED;

/**
 * @brief Current connection status reason.
 */
IOTHUB_CLIENT_CONNECTION_STATUS_REASON g_connection_status_reason = IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL;

/**
 * @brief Get the elapsed time since Epoch, on seconds.
 *
 * @return time_t contains the elapsed time since Epoch.
 */
static time_t GetTimeSinceEpochInSeconds()
{
    struct timespec timeSinceEpoch;
    ADUCPAL_clock_gettime(CLOCK_REALTIME, &timeSinceEpoch);
    return timeSinceEpoch.tv_sec;
}

/**
 * @brief Initializes the IoT Hub connection manager.
 *
 * @param handle_address A pointer to ADUC_ClientHandle data.
 * @param device_twin_callback A callback function to be invoked when receiving a device twin data.
 * @param client_handle_updated_callback A pointer to a callback function to be invoked when a device client handler has changed.
 * @param property_update_context An ADUC_PnPComponentClient_PropertyUpdate_Context object.
 *
 *  @return 'true' if success.
 */
bool IoTHub_CommunicationManager_Init(
    ADUC_ClientHandle* handle_address,
    IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK device_twin_callback,
    ADUC_COMMUNICATION_MANAGER_CLIENT_HANDLE_UPDATED_CALLBACK client_handle_updated_callback,
    ADUC_PnPComponentClient_PropertyUpdate_Context* property_update_context)
{
    IOTHUB_CLIENT_RESULT iothubInitResult;

    if (g_iothub_client_initialized)
    {
        Log_Info("Already initialized.");
        return true;
    }

    // Before invoking ANY IoTHub Device SDK functionality, IoTHub_Init must be invoked.
    if ((iothubInitResult = IoTHub_Init()) != 0)
    {
        Log_Error("IoTHub_Init failed. Error=%d", iothubInitResult);
        return false;
    }

    pthread_mutex_lock(&s_client_handle_mutex);
    g_aduc_client_handle_address = handle_address;
    pthread_mutex_unlock(&s_client_handle_mutex);
    g_device_twin_callback = device_twin_callback;
    g_property_update_context = property_update_context;
    g_iothub_client_handle_changed_callback = client_handle_updated_callback;
    g_iothub_client_initialized = true;

    return true;
}

/**
 * @brief Destroy IoTHub device client handle.
 *
 * @param deviceHandle IoTHub device client handle.
 */
static void ADUC_DeviceClient_Destroy(ADUC_ClientHandle clientHandle)
{
    if (clientHandle != NULL)
    {
        ClientHandle_Destroy(clientHandle);
    }
}

/**
 * @brief De-initialize the IoT Hub connection manager.
 */
void IoTHub_CommunicationManager_Deinit()
{
    Log_Info("IoTHub_CommunicationManager_Deinit: deinitializing communication manager");
    pthread_mutex_lock(&s_client_handle_mutex);
    if (g_aduc_client_handle_address != NULL && *g_aduc_client_handle_address != NULL)
    {
        ClientHandle_Destroy(*g_aduc_client_handle_address);
        g_aduc_client_handle_address = NULL;
    }
    pthread_mutex_unlock(&s_client_handle_mutex);
    pthread_mutex_destroy(&s_client_handle_mutex);

    if (g_iothub_client_initialized)
    {
        IoTHub_Deinit();
        g_iothub_client_initialized = false;
        Log_Info("IoTHub_CommunicationManager_Deinit: IoTHub deinitialized successfully");
    }
}

/**
 * @brief Checks whether the connection to IoT Hub is authenticated.
 */
bool IoTHub_CommunicationManager_IsAuthenticated()
{
    return g_connection_status == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED;
}

/**
 * @brief Gets the current IoT Hub connection handle.
 *
 * @return ADUC_ClientHandle object.
 */
ADUC_ClientHandle IoTHub_CommunicationManager_GetHandle()
{
    pthread_mutex_lock(&s_client_handle_mutex);
    ADUC_ClientHandle handle = (g_aduc_client_handle_address != NULL ? *g_aduc_client_handle_address : NULL);
    pthread_mutex_unlock(&s_client_handle_mutex);
    return handle;
}

/**
 * @brief Categorization of an UNAUTHENTICATED connection status event.
 *
 * Used to distinguish the benign, expected case where the Azure IoT C SDK
 * is rotating an expired SAS token (and will reconnect on its own) from
 * a real connection failure that warrants an error in the logs.
 */
typedef enum tagADUC_ConnUnauthCategory
{
    ADUC_ConnUnauth_TransientSasRenewal = 0, /**< Expected SAS token renewal; log at Info. */
    ADUC_ConnUnauth_Broken = 1, /**< Real failure; log at Error. */
} ADUC_ConnUnauthCategory;

/**
 * @brief Classification of an IoT Hub connection-status reason for the
 *        purposes of `Connection_Maintenance()` policy.
 *
 * This classifier exists so that the policy decision ("is this a transient
 * transport disconnect the SDK can recover from on the existing handle, or
 * is this something that requires us to recreate the client handle?") is a
 * pure function of the reason code and can be unit-tested directly. See
 * ADO Bug 38069154 ("ADU Agent — Restart Loop on Transient IoT Hub
 * Disconnect") for the background. Prior to that fix, the
 * `IOTHUB_CLIENT_CONNECTION_NO_NETWORK` branch unconditionally called
 * `ADUC_MethodCall_RestartAgent()` which terminated the host process and
 * relied on systemd to respawn — producing a self-reinforcing restart loop
 * on any device experiencing transient TCP RSTs (errno=104) on the
 * MQTT/TLS socket while a deployment was in progress.
 */
typedef enum tagADUC_ConnReasonClass
{
    /** Transport-layer disconnect the IoT Hub C SDK can recover from on the
     *  existing client handle (default retry policy:
     *  IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER). The agent must
     *  NOT destroy the handle and must NOT exit. */
    ADUC_ConnReason_TransientTransport = 0,
    /** Credential needs to be refreshed; the existing reauth path will
     *  destroy and recreate the client handle. */
    ADUC_ConnReason_Credential = 1,
    /** The device has been administratively disabled on the IoT Hub side.
     *  Long backoff (existing behavior: 1 hour). */
    ADUC_ConnReason_DeviceDisabled = 2,
    /** Reason code indicates the connection is healthy; no maintenance
     *  action needed. */
    ADUC_ConnReason_Ok = 3,
    /** Reason code not recognized by this version of the agent. Treated
     *  conservatively (transient-style backoff, no restart). */
    ADUC_ConnReason_Unknown = 4,
} ADUC_ConnReasonClass;

/**
 * @brief Classifies an IoT Hub connection-status reason.
 *
 * Pure function with no side effects; safe to call from tests.
 *
 * @param reason The IOTHUB_CLIENT_CONNECTION_STATUS_REASON reported by the
 *               SDK in either an AUTHENTICATED or UNAUTHENTICATED callback.
 * @return The policy class for `Connection_Maintenance()`.
 */
ADUC_ConnReasonClass IoTHub_CommunicationManager_ClassifyConnectionReason(
    IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason)
{
    switch (reason)
    {
    case IOTHUB_CLIENT_CONNECTION_NO_NETWORK:
    case IOTHUB_CLIENT_CONNECTION_NO_PING_RESPONSE:
    case IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR:
        return ADUC_ConnReason_TransientTransport;
    case IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN:
    case IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL:
    case IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED:
        return ADUC_ConnReason_Credential;
    case IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED:
        return ADUC_ConnReason_DeviceDisabled;
    case IOTHUB_CLIENT_CONNECTION_OK:
        return ADUC_ConnReason_Ok;
    default:
        return ADUC_ConnReason_Unknown;
    }
}

/**
 * @brief Categorizes an IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED event by reason.
 *
 * SAS-token expiry is part of the SDK's normal token-rotation flow (it fires
 * about every 48 minutes for symmetric-key auth) and should not be reported
 * as a broken connection. See GitHub issue #779.
 *
 * @param reason The IOTHUB_CLIENT_CONNECTION_STATUS_REASON reported alongside
 *               the UNAUTHENTICATED status.
 * @return ADUC_ConnUnauth_TransientSasRenewal for benign SAS-token rotation,
 *         ADUC_ConnUnauth_Broken otherwise.
 */
ADUC_ConnUnauthCategory IoTHub_CommunicationManager_CategorizeUnauthenticated(
    IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason)
{
    if (reason == IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN)
    {
        return ADUC_ConnUnauth_TransientSasRenewal;
    }
    return ADUC_ConnUnauth_Broken;
}

/**
 * @brief A callback use for processing the IoT Hub Client connection status changed event.
 *
 * @param status An IoT Hub connection status
 * @param status_reason The value indicates the reason that the IoT Hub connection status change.
 * @param user_context_callback Additional context used for processing this status changed event.
 */
void IoTHub_CommunicationManager_ConnectionStatus_Callback(
    IOTHUB_CLIENT_CONNECTION_STATUS status,
    IOTHUB_CLIENT_CONNECTION_STATUS_REASON status_reason,
    void* user_context_callback)
{
    UNREFERENCED_PARAMETER(user_context_callback);
    time_t now_time = GetTimeSinceEpochInSeconds();

    Log_Debug("IotHub connection status: %d, reason: %d", status, status_reason);
    switch (status)
    {
    case IOTHUB_CLIENT_CONNECTION_AUTHENTICATED:
        g_last_authenticated_time = now_time;
        g_authentication_retries = 0;
        break;
    case IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED:
        if (IoTHub_CommunicationManager_CategorizeUnauthenticated(status_reason)
            == ADUC_ConnUnauth_TransientSasRenewal)
        {
            // Expected SAS-token rotation. The SDK will refresh the token and
            // reconnect on its own; do not log an error and do not reset
            // g_first_unauthenticated_time so that a *real* outage that
            // follows still trips the "broken for N seconds" branch below.
            Log_Info("IoTHub SAS token expired; SDK will renew the connection.");
        }
        else if (g_last_authenticated_time >= g_first_unauthenticated_time)
        {
            Log_Error("IoTHub connection is broken.");
            g_first_unauthenticated_time = now_time;
        }
        else
        {
            Log_Error(
                "IoTHub connection is broken for %d seconds (will retry in %d seconds)",
                now_time - g_first_unauthenticated_time,
                g_next_authentication_attempt_time - now_time);
        }
        break;
    }

    g_connection_status = status;
    g_connection_status_reason = status_reason;
    g_last_connection_status_callback_time = now_time;
}

static IOTHUB_CLIENT_TRANSPORT_PROVIDER GetIotHubProtocolFromConfig()
{
#ifdef ADUC_GET_IOTHUB_PROTOCOL_FROM_CONFIG
    IOTHUB_CLIENT_TRANSPORT_PROVIDER transportProvider = NULL;

    const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
    if (config != NULL)
    {
        if (config->iotHubProtocol != NULL)
        {
            if (strcmp(config->iotHubProtocol, "mqtt") == 0)
            {
                transportProvider = MQTT_Protocol;
                Log_Info("IotHub Protocol: MQTT");
            }
            else if (strcmp(config->iotHubProtocol, "mqtt/ws") == 0)
            {
                transportProvider = MQTT_WebSocket_Protocol;
                Log_Info("IotHub Protocol: MQTT/WS");
            }
            else
            {
                Log_Error(
                    "Unsupported 'iotHubProtocol' value of '%s' from '" ADUC_CONF_FILE_PATH "'.",
                    config->iotHubProtocol);
            }
        }
        else
        {
            Log_Warn("Missing 'iotHubProtocol' setting from '" ADUC_CONF_FILE_PATH "'. Default to MQTT.");
            transportProvider = MQTT_Protocol;
            Log_Info("IotHub Protocol: MQTT");
        }

        ADUC_ConfigInfo_ReleaseInstance(config);
    }
    else
    {
        Log_Error("ADUC_ConfigInfo singleton hasn't been initialized.");
    }

    return transportProvider;

#else

#    ifdef ADUC_ALLOW_MQTT
    Log_Info("IotHub Protocol: MQTT");
    return MQTT_Protocol;
#    endif // ADUC_ALLOW_MQTT

#    ifdef ADUC_ALLOW_MQTT_OVER_WEBSOCKETS
    Log_Info("IotHub Protocol: MQTT/WS");
    return MQTT_WebSocket_Protocol;
#    endif // ADUC_ALLOW_MQTT_OVER_WEBSOCKETS

#endif // ADUC_GET_IOTHUB_PROTOCOL_FROM_CONFIG
}

/**
 * @brief Creates an IoTHub device client handler and register all callbacks.
 * @details should use ADUC_DeviceClient_Destroy() to uninit clientHandle
 * @param outClientHandle clientHandle to be initialized with the connection info and launchArgs
 * @param connInfo struct containing the connection information for the DeviceClient
 * @param iotHubTracingEnabled A boolean indicates whether to enable the IoTHub tracing.
 * @return true on success, false on failure
 */
static bool ADUC_DeviceClient_Create(
    ADUC_ClientHandle* outClientHandle, ADUC_ConnectionInfo* connInfo, const bool iotHubTracingEnabled)
{
    IOTHUB_CLIENT_RESULT iothubResult;
    HTTP_PROXY_OPTIONS proxyOptions;
    memset(&proxyOptions, 0, sizeof(proxyOptions));
    bool result = true;
    bool shouldSetProxyOptions = InitializeProxyOptions(&proxyOptions);

    Log_Info("Attempting to create connection to IotHub using type: %s ", ADUC_ConnType_ToString(connInfo->connType));

    IOTHUB_CLIENT_TRANSPORT_PROVIDER transportProvider = GetIotHubProtocolFromConfig();
    if (transportProvider == NULL)
    {
        result = false;
    }
    // Create a connection to IoTHub.
    else if (!ClientHandle_CreateFromConnectionString(
                 outClientHandle, connInfo->connType, connInfo->connectionString, transportProvider))
    {
        Log_Error("Failure creating IotHub device client using MQTT protocol. Check your connection string.");
        result = false;
    }
    // Sets IoTHub tracing verbosity level.
    else if (
        (iothubResult = ClientHandle_SetOption(*outClientHandle, OPTION_LOG_TRACE, &iotHubTracingEnabled))
        != IOTHUB_CLIENT_OK)
    {
        Log_Error("Unable to set IoTHub tracing option, error=%d", iothubResult);
        result = false;
    }
    else if (
        connInfo->clientCertificateString != NULL
        && (connInfo->authType == ADUC_AuthType_SASCert || connInfo->authType == ADUC_AuthType_X509)
        && (iothubResult =
                ClientHandle_SetOption(*outClientHandle, SU_OPTION_X509_CERT, connInfo->clientCertificateString))
            != IOTHUB_CLIENT_OK)
    {
        Log_Error("Unable to set client certificate, error=%d", iothubResult);
        result = false;
    }
    else if (
        shouldSetProxyOptions
        && ((iothubResult = ClientHandle_SetOption(*outClientHandle, OPTION_HTTP_PROXY, &proxyOptions))
            != IOTHUB_CLIENT_OK))
    {
        Log_Error("Could not set http proxy options, error=%d ", iothubResult);
        result = false;
    }
    else if (
        connInfo->certificateString != NULL
        && (connInfo->authType == ADUC_AuthType_NestedEdgeCert || connInfo->authType == ADUC_AuthType_X509
            || connInfo->authType == ADUC_AuthType_SASCert)
        && (iothubResult = ClientHandle_SetOption(*outClientHandle, OPTION_TRUSTED_CERT, connInfo->certificateString))
            != IOTHUB_CLIENT_OK)
    {
        Log_Error("Could not add trusted certificate, error=%d ", iothubResult);
        result = false;
    }
    else if (
        connInfo->opensslEngine != NULL
        && (connInfo->authType == ADUC_AuthType_SASCert || connInfo->authType == ADUC_AuthType_X509)
        && (iothubResult = ClientHandle_SetOption(*outClientHandle, OPTION_OPENSSL_ENGINE, connInfo->opensslEngine))
            != IOTHUB_CLIENT_OK)
    {
        Log_Error("Unable to set IotHub OpenSSL Engine, error=%d", iothubResult);
        result = false;
    }
    else if (
        connInfo->opensslPrivateKey != NULL
        && (connInfo->authType == ADUC_AuthType_SASCert || connInfo->authType == ADUC_AuthType_X509)
        && (iothubResult =
                ClientHandle_SetOption(*outClientHandle, SU_OPTION_X509_PRIVATE_KEY, connInfo->opensslPrivateKey))
            != IOTHUB_CLIENT_OK)
    {
        Log_Error("Unable to set IotHub OpenSSL Private Key, error=%d", iothubResult);
        result = false;
    }
    else if (
        connInfo->opensslEngine != NULL && connInfo->opensslPrivateKey != NULL
        && (connInfo->authType == ADUC_AuthType_SASCert || connInfo->authType == ADUC_AuthType_X509)
        && (iothubResult =
                ClientHandle_SetOption(*outClientHandle, OPTION_OPENSSL_PRIVATE_KEY_TYPE, &x509_key_from_engine))
            != IOTHUB_CLIENT_OK)
    {
        Log_Error("Unable to set IotHub OpenSSL Private Key Type, error=%d", iothubResult);
        result = false;
    }
    // Sets the name of ModelId for this PnP device.
    // This *MUST* be set before the client is connected to IoTHub.  We do not automatically connect when the
    // handle is created, but will implicitly connect to subscribe for device method and device twin callbacks below.
    else if (
        (iothubResult = ClientHandle_SetOption(*outClientHandle, OPTION_MODEL_ID, g_aduModelId)) != IOTHUB_CLIENT_OK)
    {
        Log_Error("Unable to set the Device Twin Model ID, error=%d", iothubResult);
        result = false;
    }
    // Sets the callback function that processes device twin changes from the IoTHub, which is the channel
    // that PnP Properties are transferred over.
    // This will also automatically retrieve the full twin for the application.
    else if (
        (iothubResult =
             ClientHandle_SetClientTwinCallback(*outClientHandle, g_device_twin_callback, g_property_update_context))
        != IOTHUB_CLIENT_OK)
    {
        Log_Error("Unable to set device twin callback, error=%d", iothubResult);
        result = false;
    }
    else if (
        (iothubResult = ClientHandle_SetConnectionStatusCallback(
             *outClientHandle, IoTHub_CommunicationManager_ConnectionStatus_Callback, NULL))
        != IOTHUB_CLIENT_OK)
    {
        Log_Error("Unable to set connection status callback, error=%d", iothubResult);
        result = false;
    }
    else
    {
        Log_Info("IoTHub Device Twin callback registered.");
        result = true;
    }

    if ((result == false) && (*outClientHandle != NULL))
    {
        ClientHandle_Destroy(*outClientHandle);
        *outClientHandle = NULL;
    }

    if (shouldSetProxyOptions)
    {
        UninitializeProxyOptions(&proxyOptions);
    }

    return result;
}

/**
 * @brief Scans the connection string and returns the connection type related to the string
 * @details The connection string must use the valid, correct format for the DeviceId and/or the ModuleId
 * e.g.
 * "DeviceId=some-device-id;ModuleId=some-module-id;"
 * If the connection string contains the DeviceId it is an ADUC_ConnType_Device
 * If the connection string contains the DeviceId AND the ModuleId it is an ADUC_ConnType_Module
 * @param connectionString the connection string to scan
 * @returns the connection type for @p connectionString
 */
ADUC_ConnType GetConnTypeFromConnectionString(const char* connectionString)
{
    ADUC_ConnType result = ADUC_ConnType_NotSet;

    if (connectionString == NULL)
    {
        Log_Debug("Connection string passed to GetConnTypeFromConnectionString is NULL");
        return ADUC_ConnType_NotSet;
    }

    if (ConnectionStringUtils_DoesKeyExist(connectionString, "DeviceId"))
    {
        if (ConnectionStringUtils_DoesKeyExist(connectionString, "ModuleId"))
        {
            result = ADUC_ConnType_Module;
        }
        else
        {
            result = ADUC_ConnType_Device;
        }
    }
    else
    {
        Log_Debug("DeviceId not present in connection string.");
    }

    return result;
}

/**
 * @brief Get the Connection Info from connection string, if a connection string is provided in configuration file
 *
 * @remarks This function requires that ADUC_ConfigInfo singleton has been initialized.
 *
 * @return true if connection info can be obtained
 */
bool GetConnectionInfoFromConnectionString(
    ADUC_ConnectionInfo* info,
    const char* connectionString,
    const char* const x509Cert,
    const char* const x509PrivateKey,
    const char* const opensslEngine,
    const char* const x509CaCert)
{
    bool succeeded = false;
    if (info == NULL)
    {
        Log_Error("GetConnectionInfoFromConnectionString: info parameter is NULL");
        goto done;
    }
    if (connectionString == NULL)
    {
        Log_Error("GetConnectionInfoFromConnectionString: connectionString parameter is NULL");
        goto done;
    }

    memset(info, 0, sizeof(*info));

    if (mallocAndStrcpy_s(&info->connectionString, connectionString) != 0)
    {
        goto done;
    }

    info->connType = GetConnTypeFromConnectionString(info->connectionString);

    if (info->connType == ADUC_ConnType_NotSet)
    {
        Log_Error("Connection type is invalid");
        goto done;
    }

    if (x509Cert)
    {
        Log_Info("Initializing X.509 authentication data");
        assert(x509PrivateKey);
        assert(x509CaCert);
        info->authType = ADUC_AuthType_X509;
        if (mallocAndStrcpy_s(&info->clientCertificateString, x509Cert) != 0)
        {
            goto done;
        }
        if (mallocAndStrcpy_s(&info->opensslPrivateKey, x509PrivateKey) != 0)
        {
            goto done;
        }
        if (mallocAndStrcpy_s(&info->certificateString, x509CaCert) != 0)
        {
            goto done;
        }
        if(opensslEngine) {
            if (mallocAndStrcpy_s(&info->opensslEngine, opensslEngine) != 0)
            {
                goto done;
            }
        }
        // Show x509 config info
        Log_Info("Successfully initialized X.509 authentication - connectionString:%s", &info->connectionString);
    }
    else
    {
        info->authType = ADUC_AuthType_SASToken;
    }

    succeeded = true;

done:
    return succeeded;
}

/**
 * @brief Get the Connection Info from Identity Service
 *
 * @return true if connection info can be obtained
 */
bool GetConnectionInfoFromIdentityService(ADUC_ConnectionInfo* info)
{
    bool succeeded = false;
    Log_Info("Attempting to get connection info from Identity Service (EIS)");
    if (info == NULL)
    {
        Log_Error("GetConnectionInfoFromIdentityService: info parameter is NULL");
        goto done;
    }
    memset(info, 0, sizeof(*info));

    time_t expirySecsSinceEpoch = time(NULL) + EIS_TOKEN_EXPIRY_TIME_IN_SECONDS;

    EISUtilityResult eisProvisionResult =
        RequestConnectionStringFromEISWithExpiry(expirySecsSinceEpoch, EIS_PROVISIONING_TIMEOUT, info);

    if (eisProvisionResult.err != EISErr_Ok && eisProvisionResult.service != EISService_Utils)
    {
        Log_Info(
            "Failed to provision a connection string from eis, Failed with error %s on service %s",
            EISErr_ErrToString(eisProvisionResult.err),
            EISService_ServiceToString(eisProvisionResult.service));
        goto done;
    }

    succeeded = true;
done:

    return succeeded;
}

/**
 * @brief Applies edge gateway certificate as a post-processing step after auth-specific initialization.
 *
 * @details When edgegatewayCertPath is configured in du-config.json, the gateway certificate is loaded
 * and set as the trusted certificate for validating the Edge gateway's TLS server certificate.
 * For certificate-based auth (X509, SASCert), the original authType is preserved so that client
 * certificate and private key SDK options continue to be applied. For SASCert (EIS x509), the
 * client cert is relocated from certificateString to clientCertificateString before overwriting.
 *
 * @param info Connection info struct already populated by GetConnectionInfoFromConnectionString or
 *             GetConnectionInfoFromIdentityService.
 * @return true on success, false on failure
 */
static bool ApplyEdgeGatewayCertIfConfigured(ADUC_ConnectionInfo* info)
{
    bool succeeded = false;
    const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();

    if (config == NULL || config->edgegatewayCertPath == NULL)
    {
        // No edge gateway configured — nothing to do.
        succeeded = true;
        goto done;
    }

    Log_Info("Applying edge gateway certificate from: %s", config->edgegatewayCertPath);

    char certificateString[8192];
    if (!LoadBufferWithFileContents(config->edgegatewayCertPath, certificateString, ARRAY_SIZE(certificateString)))
    {
        Log_Error("Failed to read the edge gateway certificate from path: %s", config->edgegatewayCertPath);
        goto done;
    }

    // certificateString is now reserved for trust-anchor (CA / Edge gateway) certificates.
    // The identity (client) cert always lives in clientCertificateString — populated by
    // either the direct X509 path (GetConnectionInfoFromConnectionString) or the EIS x509
    // path (RequestConnectionStringFromEISWithExpiry). So here we only need to free any
    // existing trust anchor (e.g. the IoT Hub CA cert from direct X509) before replacing
    // it with the gateway cert.
    free(info->certificateString);
    info->certificateString = NULL;

    // Store the gateway cert as the trust anchor (will be set as OPTION_TRUSTED_CERT).
    if (mallocAndStrcpy_s(&info->certificateString, certificateString) != 0)
    {
        Log_Error("Failed to copy edge gateway certificate string.");
        goto done;
    }

    // Only change authType for non-cert-based auth scenarios.
    // For X509 and SASCert, preserve the original authType so that client cert,
    // private key, and engine SDK options are still set on the IoT Hub handle.
    if (info->authType == ADUC_AuthType_SASToken || info->authType == ADUC_AuthType_NotSet)
    {
        info->authType = ADUC_AuthType_NestedEdgeCert;
    }

    succeeded = true;

done:
    ADUC_ConfigInfo_ReleaseInstance(config);
    return succeeded;
}

/**
 * @brief Gets the agent configuration information and loads it according to the provisioning scenario
 *
 * @param info the connection information that will be configured
 * @return true on success; false on failure
 */
bool GetAgentConfigInfo(ADUC_ConnectionInfo* info)
{
    bool success = false;
    if (info == NULL)
    {
        return false;
    }
    const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();

    if (config == NULL)
    {
        Log_Error("ADUC_ConfigInfo singleton hasn't been initialized.");
        goto done;
    }

    const ADUC_AgentInfo* agent = ADUC_ConfigInfo_GetAgent(config, 0);
    if (agent == NULL)
    {
        Log_Error("ADUC_ConfigInfo_GetAgent failed to get the agent information.");
        goto done;
    }

    if (strcmp(agent->connectionType, "AIS") == 0)
    {
        if (!GetConnectionInfoFromIdentityService(info))
        {
            Log_Error("Failed to get connection information from AIS.");
            goto done;
        }
    }
    else if (strcmp(agent->connectionType, "string") == 0)
    {
        if (!GetConnectionInfoFromConnectionString(info, agent->connectionData, NULL, NULL, NULL, NULL))
        {
            goto done;
        }
    }
    else if (strcmp(agent->connectionType, "X509") == 0)
    {
        if (!GetConnectionInfoFromConnectionString(
                info, agent->connectionData, agent->x509Cert, agent->x509PrivateKey, agent->opensslEngine, agent->x509CaCert))
        {
            goto done;
        }
    }
    else
    {
        Log_Error("The connection type %s is not supported", agent->connectionType);
        goto done;
    }

    // Post-processing: apply edge gateway cert if configured (for nested edge scenarios).
    // This must happen after auth-specific initialization so that the gateway cert is
    // stored as the trust anchor without clobbering client cert/key state.
    if (!ApplyEdgeGatewayCertIfConfigured(info))
    {
        goto done;
    }

    success = true;

done:
    if (!success)
    {
        ADUC_ConnectionInfo_DeAlloc(info);
    }

    ADUC_ConfigInfo_ReleaseInstance(config);

    return success;
}

/**
 * @brief Refresh the IotHub connection, then then set an IotHub client handle on every PnP sub-component.
 *
 * Note: Learn more about IotHub SAS tokens at https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-dev-guide-sas?tabs=node#sas-tokens
 *
 */
static void ADUC_Refresh_IotHub_Connection_SAS_Token()
{
    bool handleWasRecreated = false;

    pthread_mutex_lock(&s_client_handle_mutex);
    if (g_aduc_client_handle_address == NULL)
    {
        Log_Error("Invalidate operation. Must call IoTHub_CommunicationManager_Init() to initialize the manager.");
        return;
    }

    if (g_aduc_client_handle_address != NULL && *g_aduc_client_handle_address != NULL)
    {
        ADUC_DeviceClient_Destroy(*g_aduc_client_handle_address);
        *g_aduc_client_handle_address = NULL;
        if (g_iothub_client_handle_changed_callback != NULL)
        {
            g_iothub_client_handle_changed_callback(*g_aduc_client_handle_address);
        }
    }

    ADUC_ConnectionInfo info;
    memset(&info, 0, sizeof(info));
    if (!GetAgentConfigInfo(&info))
    {
        goto done;
    }

    if (!ADUC_DeviceClient_Create(g_aduc_client_handle_address, &info, true /* iotHubTracingEnabled */))
    {
        Log_Error("ADUC_DeviceClient_Create failed during SAS token refresh");
        goto done;
    }

    if (g_iothub_client_handle_changed_callback != NULL)
    {
        g_iothub_client_handle_changed_callback(*g_aduc_client_handle_address);
    }

    handleWasRecreated = true;
    Log_Info("Successfully re-authenticated the IoT Hub connection.");

done:
    pthread_mutex_unlock(&s_client_handle_mutex);

    // The reset is intentionally invoked AFTER releasing s_client_handle_mutex:
    //   1. It avoids any future deadlock if a D2C status/completion callback
    //      ever reaches back into the communication manager (e.g. via
    //      IoTHub_CommunicationManager_GetHandle()).
    //   2. The handle pointer (*g_aduc_client_handle_address) captured by
    //      previously-submitted messages is the address of the global, so it
    //      remains valid after we release the lock; the next D2C send will
    //      deref the updated pointer.
    //
    // The previous client handle was destroyed above; any reported-state
    // messages already submitted to the old handle are now stuck in
    // Waiting_For_Response because the SDK's ack callback queue went away
    // with it. Replay them on the new handle (or drop them in favor of a
    // newer pending message of the same type). This is what makes it safe
    // for the transient-disconnect path in Connection_Maintenance() to stop
    // restarting the agent on IOTHUB_CLIENT_CONNECTION_NO_NETWORK — see ADO
    // Bug 38069154 §4.2.
    if (handleWasRecreated)
    {
        ADUC_D2C_Messaging_Reset_For_Handle_Refresh();
    }

    ADUC_ConnectionInfo_DeAlloc(&info);
}

/**
 * @brief Performs an authentication to the IoTHub as needed, with exponential back-off retry logics.
 *
 */
static void Connection_Maintenance()
{
    if (IoTHub_CommunicationManager_IsAuthenticated())
    {
        return;
    }

    // Try to (re)connect to the IoT Hub if:
    //   1. The connection is broken (or unauthenticated)
    //   2. It has been long enough since the last authentication attemps
    time_t now_time = GetTimeSinceEpochInSeconds();

    if (now_time < g_next_authentication_attempt_time)
    {
        return;
    }

    int additionalDelayInSeconds = TIME_SPAN_FIFTEEN_SECONDS_IN_SECONDS;

    // If we haven't tried to connect, no need to compute the next retry time.
    // Otherwise, compute next retry time we've attempted to authenticate after the previous time.
    if (g_last_authentication_attempt_time != 0
        && g_last_authentication_attempt_time >= g_next_authentication_attempt_time)
    {
        // Decide whether to retry or not.
        // If retry needed, choose appropriate additional delay base on a nature of error.
        switch (g_connection_status_reason)
        {
        case IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED:
        case IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN:
        case IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL:
            additionalDelayInSeconds = TIME_SPAN_FIFTEEN_SECONDS_IN_SECONDS;
            break;

        case IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED:
            // If device is disabled, wait for at least 1 hour to retry.
            Log_Error("IoT Hub reported device disabled.");
            additionalDelayInSeconds = TIME_SPAN_ONE_HOUR_IN_SECONDS;
            break;

        case IOTHUB_CLIENT_CONNECTION_NO_PING_RESPONSE:
            // Could be transient error, wait for at least 5 minutes to retry.
            Log_Error("No ping response.");
            additionalDelayInSeconds = TIME_SPAN_FIVE_MINUTES_IN_SECONDS;
            break;
        case IOTHUB_CLIENT_CONNECTION_NO_NETWORK:
            // Transient transport-layer disconnect (typically a TCP RST or
            // ECONNRESET on the MQTT/TLS socket). The Azure IoT C SDK has
            // its own reconnect policy (default
            // IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF_WITH_JITTER) and will
            // attempt to reconnect on the existing client handle as long as
            // IoTHub_CommunicationManager_DoWork() keeps invoking
            // ClientHandle_DoWork(). Do NOT destroy the handle and do NOT
            // restart the host process: doing so was the cause of ADO Bug
            // 38069154 (single transient socket reset → self-reinforcing
            // systemd restart loop while a deployment was in progress).
            //
            // The original "prevent empty reported properties in module
            // twin" concern that motivated the restart is now handled
            // properly by ADUC_D2C_Messaging_Reset_For_Handle_Refresh(),
            // which is invoked from the credential-reauth path that
            // actually destroys and recreates the client handle.
            Log_Warn("No network. Treating as transient transport disconnect; SDK will reconnect.");
            additionalDelayInSeconds = TIME_SPAN_FIVE_MINUTES_IN_SECONDS;
            break;

        case IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR:
            // Could be transient error, wait for at least 5 minutes to retry.
            Log_Error("IoT Hub communication error.");
            additionalDelayInSeconds = TIME_SPAN_FIVE_MINUTES_IN_SECONDS;
            break;

        case IOTHUB_CLIENT_CONNECTION_OK:
            // No need to retry.
            return;

        default:
            Log_Debug("unhandled g_connection_status_reason case: %d", g_connection_status_reason);
            break;
        }

        // Calculate the next retry time, then continue.
        time_t nextRetryTime = ADUC_Retry_Delay_Calculator(
            additionalDelayInSeconds,
            g_authentication_retries /* current retires count */,
            ADUC_RETRY_DEFAULT_INITIAL_DELAY_MS /* initialDelayUnitMilliSecs */,
            TIME_SPAN_ONE_HOUR_IN_SECONDS,
            ADUC_RETRY_DEFAULT_MAX_JITTER_PERCENT);

        g_next_authentication_attempt_time = (nextRetryTime);
        Log_Info(
            "The connection is currently broken. Will try to authenticate in %d seconds.", nextRetryTime - now_time);
        return;
    }

    // Try to authenticate.
    g_last_authentication_attempt_time = now_time;
    g_authentication_retries++;
    ADUC_Refresh_IotHub_Connection_SAS_Token();
}

/**
 * @brief Performs the connection management tasks synchronously (in the caller's thread context).
 *
 * @remark This function may destroy the current IoT Hub client handler. Hence, it must not be called while
 *         the IoT Hub client handler is in use.
 */
void IoTHub_CommunicationManager_DoWork(void* user_context)
{
    UNREFERENCED_PARAMETER(user_context);
    Connection_Maintenance();
    ClientHandle_DoWork(*g_aduc_client_handle_address);
}
