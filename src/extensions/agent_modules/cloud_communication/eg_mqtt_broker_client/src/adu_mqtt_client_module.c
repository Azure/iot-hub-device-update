/**
 * @file adu_mqtt_client_module.c
 * @brief Implementation for the Device Update agent module for the Event Grid MQTT broker.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adu_mqtt_client_module.h"
#include "aduc/adu_agent_info_management.h"
#include "aduc/adu_communication_channel.h"
#include "aduc/adu_enrollment_management.h"
#include "aduc/adu_mosquitto_utils.h"
#include "aduc/adu_mqtt_protocol.h"
#include "aduc/adu_types.h"
#include "aduc/adu_updates_management.h"
#include "aduc/logging.h"
#include "aduc/string_c_utils.h"
#include "du_agent_sdk/agent_module_interface.h"

#include "aducpal/time.h" // time_t
#include "errno.h"
#include "string.h"
#include <mosquitto.h> // mosquitto related functions
#include <mqtt_protocol.h> // mosquitto_property

static ADUC_MQTT_CLIENT_MODULE_STATE g_moduleState = { 0 };

/**
 * @brief The contract info for the module.
 */
ADUC_AGENT_CONTRACT_INFO g_egMqttClientContractInfo = {
    "Microsoft", "Event Grid MQTT Client  Module", 1, "Microsoft/AzureEGMQTTClientModule:1"
};

/**
 * @brief Gets the extension contract info.
 * @param handle The handle to the module. This is the same handle that was returned by the Create function.
 * @return ADUC_AGENT_CONTRACT_INFO The extension contract info.
 */
const ADUC_AGENT_CONTRACT_INFO* ADUC_MQTT_Client_Module_GetContractInfo(ADUC_AGENT_MODULE_HANDLE handle)
{
    IGNORED_PARAMETER(handle);
    return &g_egMqttClientContractInfo;
}

/* Callback called when the client receives a message. */
void ADUC_MQTT_Client_OnMessage(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props)
{
    char* msg_type = NULL;

    Log_Info("on_message: Topic: %s; QOS: %d; mid: %d", msg->topic, msg->qos, msg->mid);

    // All parameters are required.
    if (mosq == NULL || obj == NULL || msg == NULL || props == NULL)
    {
        Log_Error("OnMessage: Null parameter (mosq=%p, obj=%p, msg=%p, props=%p)", mosq, obj, msg, props);
        goto done;
    }

    // Get the message type. (mt = message type)
    if (!ADU_mosquitto_read_user_property_string(props, "mt", &msg_type) || IsNullOrEmpty(msg_type))
    {
        Log_Warn("on_message: Failed to read the message type. Ignore the message.");
        goto done;
    }

    // Only support protocol version 1
    if (!ADU_mosquitto_has_user_property(props, "pid", "1"))
    {
        Log_Warn("on_message: Ignore the message. (pid != 1)");
        goto done;
    }

    // Route message to the appropriate component.

    if (strcmp(msg_type, "enr_resp") == 0)
    {
        OnMessage_enr_resp(mosq, obj, msg, props);
        goto done;
    }

    if (strcmp(msg_type, "enr_cn") == 0)
    {
        OnMessage_enr_cn(mosq, obj, msg, props);
        goto done;
    }

    if (strcmp(msg_type, "ainfo_resp") == 0)
    {
        OnMessage_ainfo_resp(mosq, obj, msg, props);
        goto done;
    }

    if (strcmp(msg_type, "upd_cn") == 0)
    {
        OnMessage_upd_cn(mosq, obj, msg, props);
        goto done;
    }

    if (strcmp(msg_type, "upd_resp") == 0)
    {
        OnMessage_upd_resp(mosq, obj, msg, props);
        goto done;
    }

    /* This blindly prints the payload, but the payload can be anything so take care. */
    Log_Info("\tPayload: %s\n", (char*)msg->payload);

done:
    free(msg_type);
}

/* Callback called when the client knows to the best of its abilities that a
 * PUBLISH has been successfully sent. For QoS 0 this means the message has
 * been completely written to the operating system. For QoS 1 this means we
 * have received a PUBACK from the broker. For QoS 2 this means we have
 * received a PUBCOMP from the broker. */
void ADUC_MQTT_Client_OnPublish(
    struct mosquitto* mosq, void* obj, int mid, int reason_code, const mosquitto_property* props)
{
    // TODO (nox-msft) - route the message to the appropriate component.
    Log_Info("on_publish: Message with mid %d has been published.", mid);
}

/**
 * @brief Callbacks for various event from the communication channel.
 */
ADUC_MQTT_CALLBACKS s_commChannelCallbacks = {
    NULL /*on_connect*/,        NULL /*on_disconnect*/,     NULL /*on_subscribe*/,
    ADUC_MQTT_Client_OnPublish, ADUC_MQTT_Client_OnMessage, NULL /*on_log*/
};

/**
 * @brief Initialize the IoTHub Client module.
 */
int ADUC_MQTT_Client_Module_Initialize(ADUC_AGENT_MODULE_HANDLE handle, void* moduleInitData)
{
    IGNORED_PARAMETER(moduleInitData);
    ADUC_MQTT_CLIENT_MODULE_STATE* state = (ADUC_MQTT_CLIENT_MODULE_STATE*)handle;
    if (state == NULL)
    {
        Log_Error("handle is NULL");
        return -1;
    }

    // Need to set ret and goto done after this to ensure proper shutdown and deinitialization.
    // TODO (nox-msft) - set log level according to global config.
    ADUC_Logging_Init(0, "eg-mqtt-client-module");

    // TODO (nox-msft) - set proper session id.
    bool success = ADUC_CommunicationChannel_Initialize("adu_client_0001", handle, &s_commChannelCallbacks);

    if (!success)
    {
        Log_Error("Failed to initialize the communication channel");
        goto done;
    }

    success = ADUC_Enrollment_Management_Initialize();

    if (!success)
    {
        Log_Error("Failed to initialize the enrollment management");
        goto done;
    }

    success = ADUC_Agent_Info_Management_Initialize();
    if (!success)
    {
        Log_Error("Failed to initialize the agent info management");
        goto done;
    }

    success = ADUC_Updates_Management_Initialize();
    if (!success)
    {
        Log_Error("Failed to initialize the updates management");
        goto done;
    }

done:
    return success ? 0 : -1;
}

// /**
//  * @brief Initialize the IoTHub Client module.
// */
// int ADUC_MQTT_Client_Module_Initialize(ADUC_AGENT_MODULE_HANDLE handle, void* moduleInitData)
// {
//     IGNORED_PARAMETER(moduleInitData);
//     ADUC_MQTT_CLIENT_MODULE_STATE* state = (ADUC_MQTT_CLIENT_MODULE_STATE*)handle;
//     if (state == NULL)
//     {
//         Log_Error("handle is NULL");
//         return -1;
//     }

//     ADUC_Result result = { ADUC_GeneralResult_Failure };

//     // Need to set ret and goto done after this to ensure proper shutdown and deinitialization.
//     // TODO (nox-msft) - set log level according to global config.
//     ADUC_Logging_Init(0, "eg-mqtt-client-module");

//     bool success = ADUC_CommmunicationChannelState_Init();

//     int mqtt_res = 0;
//     bool use_OS_cert = false;
//     if (!ReadMqttBrokerSettings(&state->mqttSettings))
//     {
//         SetError(-1, "Failed to read Azure DPS2 MQTT settings");
//         goto done;
//     }

//     if (!PrepareADUMQTTTopics())
//     {
//         goto done;
//     }

//     Log_Info("Initialize Mosquitto library");
//     mosquitto_lib_init();

//     // TODO (noxt-msft) : change session name.
//     state->mqttClient = mosquitto_new(
//         "mock_session_0001", state->mqttSettings.cleanSession, &g_moduleState);
//     if (!state->mqttClient)
//     {
//         //Log_Error("Failed to create Mosquitto client");
//         SetError(-1, "Failed to create Mosquitto client");
//         return -1; // Initialization failed
//     }

//     mqtt_res = mosquitto_int_option(
//         state->mqttClient, MOSQ_OPT_PROTOCOL_VERSION, state->mqttSettings.mqttVersion);
//     if (mqtt_res != MOSQ_ERR_SUCCESS)
//     {
//         Log_Error("Failed to set MQTT protocol version (%d)", mqtt_res);
//         goto done;
//     }

//     if (!IsNullOrEmpty(state->mqttSettings.username))
//     {
//         mqtt_res = mosquitto_username_pw_set(state->mqttClient, state->mqttSettings.username, state->mqttSettings.password);
//     }

//     if (state->mqttSettings.useTLS)
//     {
//         use_OS_cert = IsNullOrEmpty(state->mqttSettings.caFile);
//         if (use_OS_cert)
//         {
//             mqtt_res = mosquitto_int_option(state->mqttClient, MOSQ_OPT_TLS_USE_OS_CERTS, true);
//             if (mqtt_res != MOSQ_ERR_SUCCESS)
//             {
//                 Log_Error("Failed to set TLS use OS certs (%d)", mqtt_res);
//                 // TODO (nox-msft) - Handle all error codes.
//                 goto done;
//             }
//         }

//         // TODO (nox-msft) - Set X.509 certificate here (if using certificate-based authentication)
//         mqtt_res = mosquitto_tls_set(
//             state->mqttClient,
//             state->mqttSettings.caFile,
//             use_OS_cert ? "/etc/ssl/certs" : NULL,
//             state->mqttSettings.certFile,
//             state->mqttSettings.keyFile,
//             NULL);

//         if (mqtt_res != MOSQ_ERR_SUCCESS)
//         {
//             // Handle all error codes.
//             switch (mqtt_res)
//             {
//             case MOSQ_ERR_INVAL:
//                 Log_Error("Invalid parameter");
//                 break;

//             case MOSQ_ERR_NOMEM:
//                 Log_Error("Out of memory");
//                 break;

//             default:
//                     Log_Error("Failed to set TLS settings (%d)", mqtt_res);
//                 break;
//             }

//             goto done;
//         }
//     }

//     // Register callbacks.
//     mosquitto_connect_v5_callback_set(state->mqttClient, ADUC_MQTT_Client_OnConnect /*ADPS_OnConnect_WithSubscribe*/);
//     mosquitto_disconnect_v5_callback_set(state->mqttClient, ADUC_MQTT_Client_OnDisconnect);
//     mosquitto_subscribe_v5_callback_set(state->mqttClient, ADUC_MQTT_Client_OnSubscribe);
//     mosquitto_publish_v5_callback_set(state->mqttClient, ADUC_MQTT_Client_OnPublish);
//     mosquitto_message_v5_callback_set(state->mqttClient, ADUC_MQTT_Client_OnMessage);
//     mosquitto_log_callback_set(state->mqttClient, ADUC_MQTT_Client_OnLog);

//     state->initialized = true;
//     result.ResultCode = ADUC_GeneralResult_Success;
//     result.ExtendedResultCode = 0;

// done:
//     return result.ResultCode == ADUC_GeneralResult_Success ? 0 : -1;
// }

// /**
//  * @brief Ensure that the communication channel to the DU service is valid.
//  *
//  * @remark This function requires that the MQTT client is initialized.
//  * @remark This function requires that the MQTT connection settings are valid.
//  *
//  * @return true if the communication channel state is ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_CONNECTED.
//  */
// bool ADUC_EnsureValidCommunicationChannel()
// {
//     int mqtt_res = MOSQ_ERR_UNKNOWN;
//     if (g_moduleState.commChannelState == ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_CONNECTED)
//     {
//         return true;
//     }

//     if (!g_moduleState.initialized || g_moduleState.mqttClient == NULL)
//     {
//         Log_Debug("MQTT client is not initialized");
//         return false;
//     }

//     if (g_moduleState.commChannelState == ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_CONNECTING)
//     {
//         // TODO (nox-msft) : Check for timeout
//         // If time out, set state to UNKNOWN and try again.
//         // If not time out, let's wait, return false.
//         return false;
//     }

//     if (g_moduleState.commChannelState == ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_DISCONNECTED)
//     {
//         // TODO (nox-msft) : Check if the disconnect was due to an non-recoverable error.
//         //   If so, do nothing and return false.
//         //
//         //   If the error is recoverable, set state to UNKNOWN and try again when the nextRetryTimestamp is reached.
//         //
//         // For now, let's try to reconnect after 5 seconds.
//         if (g_moduleState.commChannelStateChangeTime + 5 > GetTimeSinceEpochInSeconds())
//         {
//             return false;
//         }
//         else
//         {
//             ADUC_SetCommunicationChannelState(ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_UNKNOWN);
//         }
//     }

//     if (g_moduleState.commChannelState == ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_UNKNOWN)
//     {
//         // Connect to an MQTT broker.
//         // It is valid to use this function for clients using all MQTT protocol versions.
//         //
//         // If you need to set MQTT v5 CONNECT properties, use <mosquitto_connect_bind_v5>
//         // instead.
//         mqtt_res = mosquitto_connect(
//             g_moduleState.mqttClient,
//             g_moduleState.mqttSettings.hostname,
//             (int)g_moduleState.mqttSettings.tcpPort,
//             (int)g_moduleState.mqttSettings.keepAliveInSeconds);

//         switch (mqtt_res)
//         {
//         case MOSQ_ERR_SUCCESS:
//             g_moduleState.commChannelLastAttemptTime = ADUC_SetCommunicationChannelState(ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_CONNECTING);
//             break;
//         case MOSQ_ERR_INVAL:
//             Log_Error("Invalid parameter (client: %d, host:%s, port:%d, keepalive:%d)", g_moduleState.mqttClient, g_moduleState.mqttSettings.hostname, g_moduleState.mqttSettings.tcpPort, g_moduleState.mqttSettings.keepAliveInSeconds);
//             break;
//         case MOSQ_ERR_ERRNO:
//             {
//                 int errno_copy = errno;
//                 if (mqtt_res == MOSQ_ERR_ERRNO)
//                 {
//                     // TODO (nox-msft) - use FormatMessage() on WinIoT
//                     Log_Error("mosquitto_connect failed (%d) - %s", mqtt_res, strerror(errno_copy));
//                 }
//                 else
//                 {
//                     Log_Error("mosquitto_connect failed (%d)", mqtt_res);
//                 }
//                 break;
//             }
//         default:
//             Log_Error("mosquitto_connect failed (%d)", mqtt_res);
//             break;
//         }
//     }

//     return false;
// }

/**
 * @brief Deinitialize the IoTHub Client module.
 *
 * @param module The handle to the module. This is the same handle that was returned by the Create function.
 *
 * @return int 0 on success.
 */
int ADUC_MQTT_Client_Module_Deinitialize(ADUC_AGENT_MODULE_HANDLE module)
{
    Log_Info("Deinitialize");

    ADUC_CommunicationChannel_Deinitialize();

    ADUC_Logging_Uninit();
    return 0;
}

/**
 * @brief Create a Device Update Agent Module for IoT Hub PnP Client.
 *
 * @return ADUC_AGENT_MODULE_HANDLE The handle to the module.
 */
ADUC_AGENT_MODULE_HANDLE ADUC_MQTT_Client_Module_Create()
{
    ADUC_AGENT_MODULE_HANDLE handle = &g_moduleState;
    return handle;
}

/*
 * @brief Destroy the Device Update Agent Module for IoT Hub PnP Client.
 * @param handle The handle to the module. This is the same handle that was returned by the Create function.
 */
void ADUC_MQTT_Client_Module_Destroy(ADUC_AGENT_MODULE_HANDLE handle)
{
    IGNORED_PARAMETER(handle);
    Log_Info("Destroy");
}

#ifdef _INTER_MODULE_COMMAND_ENABLED_
void OnCommandReceived_DPSModule(
    const char* commandName, const char* commandPayload, size_t payloadSize, void* context)
{
    IGNORED_PARAMETER(context);
    Log_Info("Received command %s with payload %s (size:%d)", commandName, commandPayload, payloadSize);
}

bool Subscribe_DPSModuleCommands()
{
    // TODO (nox-msft): subscribe to DPS module commands.
    // return InterModuleCommand_Subscribe("module.microsoft.dpsclient.ipcCommands", OnCommandReceived, NULL);
    return true;
}

void UnsubScribe_DPSModuleCommands()
{
    // TODO (nox-msft): unsubscribe to DPS module commands.
    // InterModuleCommand_Unsubscribe("module.microsoft.dpsclient.ipcCommands", OnCommandReceived, NULL);
}

void OnCommandReceived_ADUMqttClientModule(
    const char* commandName, const char* commandPayload, size_t payloadSize, void* context)
{
    IGNORED_PARAMETER(context);
    Log_Info("Received command %s with payload %s (size:%d)", commandName, commandPayload, payloadSize);
}

bool Subscribe_ADUClientModuleCommands()
{
    // TODO (nox-msft): subscribe to DPS module commands.
    // return InterModuleCommand_Subscribe("module.microsoft.adumqttclient.ipcCommands", OnCommandReceived_ADUMqttClientModule, NULL);
    return true;
}

void Unsubscribe_ADUClientModuleCommands()
{
    // TODO (nox-msft): unsubscribe to DPS module commands.
    // InterModuleCommand_Unsubscribe("module.microsoft.adumqttclient.ipcCommands", OnCommandReceived_ADUMqttClientModule, NULL);
}

#endif

/**
 * @brief Perform the work for the extension. This must be a non-blocking operation.
 *
 * @return ADUC_Result
 */
int ADUC_MQTT_Client_Module_DoWork(ADUC_AGENT_MODULE_HANDLE handle)
{
    ADUC_CommunicationChannel_DoWork();
    ADUC_Enrollment_Management_DoWork();
    ADUC_Agent_Info_Management_DoWork();
    ADUC_Updates_Management_DoWork();
    return 0;
}

/**
 * @brief Get the Data object for the specified key.
 *
 * @param handle agent module handle
 * @param dataType data type
 * @param key data key/name
 * @param buffer return buffer (call must free the memory of the return value once done with it)
 * @param size return size of the return value
 * @return int 0 on success
*/
int ADUC_MQTT_Client_Module_GetData(
    ADUC_AGENT_MODULE_HANDLE handle, ADUC_MODULE_DATA_TYPE dataType, const char* key, void** buffer, int* size)
{
    IGNORED_PARAMETER(handle);
    IGNORED_PARAMETER(dataType);
    IGNORED_PARAMETER(key);
    IGNORED_PARAMETER(buffer);
    IGNORED_PARAMETER(size);
    int ret = 0;
    return ret;
}

ADUC_AGENT_MODULE_INTERFACE ADUC_MQTT_Client_ModuleInterface = { &g_moduleState,
                                                                 ADUC_MQTT_Client_Module_Create,
                                                                 ADUC_MQTT_Client_Module_Destroy,
                                                                 ADUC_MQTT_Client_Module_GetContractInfo,
                                                                 ADUC_MQTT_Client_Module_DoWork,
                                                                 ADUC_MQTT_Client_Module_Initialize,
                                                                 ADUC_MQTT_Client_Module_Deinitialize,
                                                                 ADUC_MQTT_Client_Module_GetData };
