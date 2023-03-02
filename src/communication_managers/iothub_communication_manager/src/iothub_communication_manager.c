/**
 * @file iothub_communication_manager.c
 * @brief Implements the IoT Hub communication manager utility.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/adu_types.h"
#include "aduc/client_handle_helper.h"
#include "aduc/config_utils.h"
#include "aduc/connection_string_utils.h" // ConnectionStringUtils_DoesKeyExist
#include "aduc/https_proxy_utils.h"
#include "aduc/iothub_communication_manager.h"
#include "aduc/logging.h"
#include "aduc/retry_utils.h"
#include "aduc/string_c_utils.h" // LoadBufferWithFileContents
#include <azure_c_shared_utility/shared_util_options.h>

#include "eis_utils.h"

#include <iothub.h>
#include <iothub_client_options.h>

#ifdef ADUC_ALLOW_MQTT
#    include <iothubtransportmqtt.h>
#endif

#ifdef ADUC_ALLOW_MQTT_OVER_WEBSOCKETS
#    include <iothubtransportmqtt_websockets.h>
#endif

#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h> // strtol
#include <sys/stat.h>
#include <unistd.h>

/**
 * @brief A pointer to ADUC_ClientHandle data. This must be initialize by the component that creates the IoT Hub connection.
 */
static ADUC_ClientHandle* g_aduc_client_handle_address = NULL;

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
static const char g_aduModelId[] = "dtmi:azure:iot:deviceUpdateModel;2";

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
    clock_gettime(CLOCK_REALTIME, &timeSinceEpoch);
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

    g_aduc_client_handle_address = handle_address;
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
    if (g_aduc_client_handle_address != NULL && *g_aduc_client_handle_address != NULL)
    {
        ClientHandle_Destroy(*g_aduc_client_handle_address);
        g_aduc_client_handle_address = NULL;
    }

    if (g_iothub_client_initialized)
    {
        IoTHub_Deinit();
        g_iothub_client_initialized = false;
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
    return (g_aduc_client_handle_address != NULL ? g_aduc_client_handle_address : NULL);
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
        if (g_last_authenticated_time >= g_first_unauthenticated_time)
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

    ADUC_ConfigInfo config;
    if (ADUC_ConfigInfo_Init(&config, ADUC_CONF_FILE_PATH))
    {
        if (config.iotHubProtocol != NULL)
        {
            if (strcmp(config.iotHubProtocol, "mqtt") == 0)
            {
                transportProvider = MQTT_Protocol;
                Log_Info("IotHub Protocol: MQTT");
            }
            else if (strcmp(config.iotHubProtocol, "mqtt/ws") == 0)
            {
                transportProvider = MQTT_WebSocket_Protocol;
                Log_Info("IotHub Protocol: MQTT/WS");
            }
            else
            {
                Log_Error(
                    "Unsupported 'iotHubProtocol' value of '%s' from '" ADUC_CONF_FILE_PATH "'.",
                    config.iotHubProtocol);
            }
        }
        else
        {
            Log_Warn("Missing 'iotHubProtocol' setting from '" ADUC_CONF_FILE_PATH "'. Default to MQTT.");
            transportProvider = MQTT_Protocol;
            Log_Info("IotHub Protocol: MQTT");
        }

        ADUC_ConfigInfo_UnInit(&config);
    }
    else
    {
        Log_Error("Failed to initialize config file '" ADUC_CONF_FILE_PATH "'.");
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
    HTTP_PROXY_OPTIONS proxyOptions = {};
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
        connInfo->certificateString != NULL && connInfo->authType == ADUC_AuthType_SASCert
        && (iothubResult = ClientHandle_SetOption(*outClientHandle, SU_OPTION_X509_CERT, connInfo->certificateString))
            != IOTHUB_CLIENT_OK)
    {
        Log_Error("Unable to set IotHub certificate, error=%d", iothubResult);
        result = false;
    }
    else if (
        shouldSetProxyOptions
        && (iothubResult =
                ClientHandle_SetOption(*outClientHandle, OPTION_HTTP_PROXY, &proxyOptions) != IOTHUB_CLIENT_OK))
    {
        Log_Error("Could not set http proxy options, error=%d ", iothubResult);
        result = false;
    }
    else if (
        connInfo->certificateString != NULL && connInfo->authType == ADUC_AuthType_NestedEdgeCert
        && (iothubResult = ClientHandle_SetOption(*outClientHandle, OPTION_TRUSTED_CERT, connInfo->certificateString))
            != IOTHUB_CLIENT_OK)
    {
        Log_Error("Could not add trusted certificate, error=%d ", iothubResult);
        result = false;
    }
    else if (
        connInfo->opensslEngine != NULL && connInfo->authType == ADUC_AuthType_SASCert
        && (iothubResult = ClientHandle_SetOption(*outClientHandle, OPTION_OPENSSL_ENGINE, connInfo->opensslEngine))
            != IOTHUB_CLIENT_OK)
    {
        Log_Error("Unable to set IotHub OpenSSL Engine, error=%d", iothubResult);
        result = false;
    }
    else if (
        connInfo->opensslPrivateKey != NULL && connInfo->authType == ADUC_AuthType_SASCert
        && (iothubResult =
                ClientHandle_SetOption(*outClientHandle, SU_OPTION_X509_PRIVATE_KEY, connInfo->opensslPrivateKey))
            != IOTHUB_CLIENT_OK)
    {
        Log_Error("Unable to set IotHub OpenSSL Private Key, error=%d", iothubResult);
        result = false;
    }
    else if (
        connInfo->opensslEngine != NULL && connInfo->opensslPrivateKey != NULL
        && connInfo->authType == ADUC_AuthType_SASCert
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
static ADUC_ConnType GetConnTypeFromConnectionString(const char* connectionString)
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
 * @return true if connection info can be obtained
 */
static bool GetConnectionInfoFromConnectionString(ADUC_ConnectionInfo* info, const char* connectionString)
{
    bool succeeded = false;
    if (info == NULL)
    {
        goto done;
    }
    if (connectionString == NULL)
    {
        goto done;
    }

    memset(info, 0, sizeof(*info));

    char certificateString[8192];

    if (mallocAndStrcpy_s(&info->connectionString, connectionString) != 0)
    {
        goto done;
    }

    info->connType = GetConnTypeFromConnectionString(info->connectionString);

    if (info->connType == ADUC_ConnType_NotSet)
    {
        Log_Error("Connection string is invalid");
        goto done;
    }

    info->authType = ADUC_AuthType_SASToken;

    // Optional: The certificate string is needed for Edge Gateway connection.
    ADUC_ConfigInfo config = {};
    if (ADUC_ConfigInfo_Init(&config, ADUC_CONF_FILE_PATH) && config.edgegatewayCertPath != NULL)
    {
        if (!LoadBufferWithFileContents(config.edgegatewayCertPath, certificateString, ARRAY_SIZE(certificateString)))
        {
            Log_Error("Failed to read the certificate from path: %s", config.edgegatewayCertPath);
            goto done;
        }

        if (mallocAndStrcpy_s(&info->certificateString, certificateString) != 0)
        {
            Log_Error("Failed to copy certificate string.");
            goto done;
        }

        info->authType = ADUC_AuthType_NestedEdgeCert;
    }

    succeeded = true;

done:
    ADUC_ConfigInfo_UnInit(&config);
    return succeeded;
}

/**
 * @brief Get the Connection Info from Identity Service
 *
 * @return true if connection info can be obtained
 */
static bool GetConnectionInfoFromIdentityService(ADUC_ConnectionInfo* info)
{
    bool succeeded = false;
    if (info == NULL)
    {
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
 * @brief Gets the agent configuration information and loads it according to the provisioning scenario
 *
 * @param info the connection information that will be configured
 * @return true on success; false on failure
 */
static bool GetAgentConfigInfo(ADUC_ConnectionInfo* info)
{
    bool success = false;
    ADUC_ConfigInfo config = {};
    if (info == NULL)
    {
        return false;
    }

    if (!ADUC_ConfigInfo_Init(&config, ADUC_CONF_FILE_PATH))
    {
        Log_Error("No connection string set from launch arguments or configuration file");
        goto done;
    }

    const ADUC_AgentInfo* agent = ADUC_ConfigInfo_GetAgent(&config, 0);
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
        if (!GetConnectionInfoFromConnectionString(info, agent->connectionData))
        {
            goto done;
        }
    }
    else
    {
        Log_Error("The connection type %s is not supported", agent->connectionType);
        goto done;
    }

    success = true;

done:
    if (!success)
    {
        ADUC_ConnectionInfo_DeAlloc(info);
    }

    ADUC_ConfigInfo_UnInit(&config);

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

    ADUC_ConnectionInfo info = {};
    if (!GetAgentConfigInfo(&info))
    {
        goto done;
    }

    if (!ADUC_DeviceClient_Create(g_aduc_client_handle_address, &info, true /* iotHubTracingEnabled */))
    {
        Log_Error("ADUC_DeviceClient_Create failed");
        goto done;
    }

    if (g_iothub_client_handle_changed_callback != NULL)
    {
        g_iothub_client_handle_changed_callback(*g_aduc_client_handle_address);
    }

    Log_Info("Successfully re-authenticated the IoT Hub connection.");

done:

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
            // Could be transient error, wait for at least 5 minutes to retry.
            Log_Error("No network.");
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
    Connection_Maintenance();
    ClientHandle_DoWork(*g_aduc_client_handle_address);
}
