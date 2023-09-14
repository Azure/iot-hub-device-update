/**
 * @file adps2-client-module.c
 * @brief Implementation for the device provisioning using Azure DPS V2.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adps_gen2.h"
#include "aduc/adu_types.h"
#include "aduc/config_utils.h"
#include "aduc/logging.h"
#include "aduc/mqtt_setup.h"
#include "aduc/string_c_utils.h"
#include "du_agent_sdk/agent_module_interface.h"

#include <aducpal/time.h> // clock_gettime
#include <mosquitto.h> // mosquitto related functions

typedef enum ADPS_REGISTER_STATE_TAG
{
    ADPS_REGISTER_STATE_NONE = 0,
    ADPS_REGISTER_STATE_REGISTERING = 1,
    ADPS_REGISTER_STATE_REGISTERED = 2,
    ADPS_REGISTER_STATE_FAILED = 3,
} ADPS_REGISTER_STATE;

/**
 * @brief The module state.
 */
typedef struct ADPS_MQTT_CLIENT_MODULE_STATE_TAG
{
    struct mosquitto* mqttClient; //!< Mosquitto client
    bool initialized; //!< Module is initialized

    bool connected; //!< Device is connected to DPS
    time_t lastConnectedTime; //!< Last time connected to DPS

    bool subscribed; //!< Device is subscribed to DPS topics

    ADPS_REGISTER_STATE registerState; //!< Registration state

    char* regRequestId; //!< Registration request ID
    time_t lastErrorTime; //!< Last time an error occurred
    ADUC_Result lastAducResult; //!< Last ADUC result
    int lastError; //!< Last error code
    AZURE_DPS_2_MQTT_SETTINGS settings; //!< DPS settings
    time_t nextOperationTime; //!< Next time to perform an operation

} ADPS_MQTT_CLIENT_MODULE_STATE;

static ADPS_MQTT_CLIENT_MODULE_STATE g_moduleState = { 0 };

/**
 * @brief The contract info for the module.
 */
ADUC_AGENT_CONTRACT_INFO g_adpsMqttClientContractInfo = {
    "Microsoft", "Azure DPS2 MQTT Client  Module", 1, "Microsoft/AzureDPS2MQTTClientModule:1"
};

#ifndef MIN
#    define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

static time_t GetTimeSinceEpochInSeconds()
{
    struct timespec timeSinceEpoch;
    ADUCPAL_clock_gettime(CLOCK_REALTIME, &timeSinceEpoch);
    return timeSinceEpoch.tv_sec;
}

/* Example config file:
static const char* testADSP2Config =
    R"({)"
        R"("schemaVersion": "2.0",)"
        R"("aduShellTrustedUsers": [)"
        R"("adu")"
        R"(],)"
        R"("manufacturer": "contoso",)"
        R"("model": "espresso-v1",)"
        R"("agents" : [{)"
            R"("name": "main",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
            R"("connectionType": "ADPS2/MQTT",)"
            R"("connectionData": {)"
              R"("dps" : {)"
                R"("idScope" : "0ne0123456789abcdef",)"
                R"("registrationId" : "adu-dps-client-unit-test-device",)"
                R"("apiVersion" : "2021-06-01",)"
                R"("globalDeviceEndpoint" : "global.azure-devices-provisioning.net",)"
                R"("tcpPort" : 8883,)"
                R"("useTLS": true,)"
                R"("cleanSession" : false,)"
                R"("keepAliveInSeconds" : 3600,)"
                R"("clientId" : "adu-dps-client-unit-test-device",)"
                R"("userName" : "adu-dps-client-unit-test-user-1",)"
                R"("password" : "adu-dps-client-unit-test-password-1",)"
                R"("caFile" : "adu-dps-client-unit-test-ca-1",)"
                R"("certFile" : "adu-dps-client-unit-test-cert-1.pem",)"
                R"("keyFile" : "adu-dps-client-unit-test-key-1.pem",)"
                R"("keyFilePassword" : "adu-dps-client-unit-test-key-password-1")"
              R"(})"
            R"(})"
            R"(},)"
            R"("manufacturer": "contoso",)"
            R"("model": "espresso-v1")"
        R"(}])"
    R"(})";
*/

/**
 * @brief Gets the extension contract info.
 * @param handle The handle to the module. This is the same handle that was returned by the Create function.
 * @return ADUC_AGENT_CONTRACT_INFO The extension contract info.
 */
const ADUC_AGENT_CONTRACT_INFO* ADPS_MQTT_Client_Module_GetContractInfo(ADUC_AGENT_MODULE_HANDLE handle)
{
    IGNORED_PARAMETER(handle);
    return &g_adpsMqttClientContractInfo;
}

enum ADPS_ERROR
{
    ADPS_ERROR_NONE = 0,
    ADPS_ERROR_INVALID_PARAMETER = 1,
    ADPS_ERROR_INVALID_STATE = 2,
    ADPS_ERROR_OUT_OF_MEMORY = 3,
    ADPS_ERROR_MQTT_ERROR = 4,
    ADPS_ERROR_DPS_ERROR = 5,
    ADPS_ERROR_TIMEOUT = 6,
    ADPS_ERROR_UNKNOWN = 7,
};

/**
 * @brief Set the last error code and log the error message.
 */
void SetError(int error, const char* message)
{
    g_moduleState.lastError = error;
    g_moduleState.lastErrorTime = GetTimeSinceEpochInSeconds();
    if (message != NULL)
    {
        Log_Error(message);
    }
}

/* Callback called when the client receives a CONNACK message from the broker. */
void ADPS_OnConnect(struct mosquitto* mosq, void* obj, int reason_code, int flags, const mosquitto_property* props)
{
    ADPS_MQTT_CLIENT_MODULE_STATE* state = (ADPS_MQTT_CLIENT_MODULE_STATE*)obj;
    state->connected = true;

    /* Print out the connection result. mosquitto_connack_string() produces an
   * appropriate string for MQTT v3.x clients, the equivalent for MQTT v5.0
   * clients is mosquitto_reason_string().
   */
    if ((int)state->settings.mqttSettings.mqttVersion == MQTT_PROTOCOL_V5)
    {
        Log_Info("on_connect: %s", mosquitto_reason_string(reason_code));
    }
    else
    {
        Log_Info("on_connect: %s", mosquitto_connack_string(reason_code));
    }

    //   if (reason_code != 0)
    //   {
    //     keep_running = 0;
    //     /* If the connection fails for any reason, we don't want to keep on
    //      * retrying in this example, so disconnect. Without this, the client
    //      * will attempt to reconnect. */
    //     int rc;
    //     if ((rc = mosquitto_disconnect_v5(mosq, reason_code, NULL)) != MOSQ_ERR_SUCCESS)
    //     {
    //       Log_Error("Failure on disconnect: %s", mosquitto_strerror(rc));
    //     }
    //   }
}

/* Callback called when the broker has received the DISCONNECT command and has disconnected the
 * client. */
void ADPS_OnDisconnect(struct mosquitto* mosq, void* obj, int rc, const mosquitto_property* props)
{
    connection_ok = 0;
    Log_Info("on_disconnect: reason=%s", mosquitto_strerror(rc));
}

/* Callback called when the client receives a message. */
void ADPS_OnMessage(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props)
{
    Log_Info("on_message: Topic: %s; QOS: %d; mid: %d", msg->topic, msg->qos, msg->mid);

    //ADPS_MQTT_CLIENT_MODULE_STATE* state = (ADPS_MQTT_CLIENT_MODULE_STATE*)obj;

    // TODO (nox-msft) : Handle the message.

    // if (state != NULL && state->handle_message != NULL)
    // {
    //     client_obj->handle_message(mosq, msg, props);
    // }

    /* This blindly prints the payload, but the payload can be anything so take care. */
    Log_Info("\tPayload: %s\n", (char*)msg->payload);
}

/* Callback called when the client knows to the best of its abilities that a
 * PUBLISH has been successfully sent. For QoS 0 this means the message has
 * been completely written to the operating system. For QoS 1 this means we
 * have received a PUBACK from the broker. For QoS 2 this means we have
 * received a PUBCOMP from the broker. */
void ADPS_OnPublish(struct mosquitto* mosq, void* obj, int mid, int reason_code, const mosquitto_property* props)
{
    Log_Info("on_publish: Message with mid %d has been published.", mid);
}

/* Callback called when the broker sends a SUBACK in response to a SUBSCRIBE. */
void ADPS_OnSubscribe(
    struct mosquitto* mosq, void* obj, int mid, int qos_count, const int* granted_qos, const mosquitto_property* props)
{
    Log_Info("on_subscribe: Subscribed with mid %d; %d topics.", mid, qos_count);

    /* In this example we only subscribe to a single topic at once, but a
   * SUBSCRIBE can contain many topics at once, so this is one way to check
   * them all. */
    for (int i = 0; i < qos_count; i++)
    {
        Log_Info("\tQoS %d\n", granted_qos[i]);
    }
}

void ADPS_OnLog(struct mosquitto* mosq, void* obj, int level, const char* str)
{
#ifndef LOG_ALL_MOSQUITTO
    if (level == MOSQ_LOG_ERR || strstr(str, "PINGREQ") != NULL || strstr(str, "PINGRESP") != NULL)
    {
        Log_Info("%s", str);
    }
#else
    {
        char* log_level_str;
        switch (level)
        {
        case MOSQ_LOG_DEBUG:
            log_level_str = "DEBUG";
            break;
        case MOSQ_LOG_INFO:
            log_level_str = "INFO";
            break;
        case MOSQ_LOG_NOTICE:
            log_level_str = "NOTICE";
            break;
        case MOSQ_LOG_WARNING:
            log_level_str = "WARNING";
            break;
        case MOSQ_LOG_ERR:
            log_level_str = "ERROR";
            break;
        default:
            log_level_str = "";
            break;
        }
        Log_Info("[%s] %s", log_level_str, str);
    }
#endif
}

// /* Callback called when the client receives a CONNACK message from the broker and we want to
//  * subscribe on connect. */
// void ADPS_OnConnect_WithSubscribe(
//     struct mosquitto* mosq,
//     void* obj,
//     int reason_code,
//     int flags,
//     const mosquitto_property* props)
// {
//   ADPS_On_Connect(mosq, obj, reason_code, flags, props);

//   connection_ok = 1;
//   int result;

//   /* Making subscriptions in the on_connect() callback means that if the
//    * connection drops and is automatically resumed by the client, then the
//    * subscriptions will be recreated when the client reconnects. */
//   if (keep_running
//       && (result = mosquitto_subscribe_v5(mosq, NULL, SUB_TOPIC, QOS_LEVEL, 0, NULL))
//           != MOSQ_ERR_SUCCESS)
//   {
//     LOG_ERROR("Failed to subscribe: %s", mosquitto_strerror(result));
//     keep_running = 0;
//     /* We might as well disconnect if we were unable to subscribe */
//     if ((result = mosquitto_disconnect_v5(mosq, reason_code, props)) != MOSQ_ERR_SUCCESS)
//     {
//         LOG_ERROR("Failed to disconnect: %s", mosquitto_strerror(result));
//     }
//   }
// }

#define REQUIRED_TLS_SET_CERT_PATH "L"

/**
 * @brief Initialize the IoTHub Client module.
*/
int ADPS_MQTT_Client_Module_Initialize(ADUC_AGENT_MODULE_HANDLE handle, void* moduleInitData)
{
    IGNORED_PARAMETER(handle);
    IGNORED_PARAMETER(moduleInitData);

    ADUC_Result result = { ADUC_GeneralResult_Failure };

    int mqtt_res = 0;
    bool use_OS_cert = false;

    // Need to set ret and goto done after this to ensure proper shutdown and deinitialization.
    // TODO (nox-msft) - set log level according to global config.
    ADUC_Logging_Init(0, "adps2-mqtt-client-module");

    if (!ReadAzureDPS2MqttSettings(&g_moduleState.settings))
    {
        SetError(-1, "Failed to read Azure DPS2 MQTT settings");
        goto done;
    }

    Log_Info("Initialize Mosquitto library");
    mosquitto_lib_init();

    g_moduleState.mqttClient = mosquitto_new(
        g_moduleState.settings.registrationId, g_moduleState.settings.mqttSettings.cleanSession, &g_moduleState);
    if (!g_moduleState.mqttClient)
    {
        //Log_Error("Failed to create Mosquitto client");
        SetError(-1, "Failed to create Mosquitto client");
        return -1; // Initialization failed
    }

    mqtt_res = mosquitto_int_option(
        g_moduleState.mqttClient, MOSQ_OPT_PROTOCOL_VERSION, g_moduleState.settings.mqttSettings.mqttVersion);
    if (mqtt_res != MOSQ_ERR_SUCCESS)
    {
        Log_Error("Failed to set MQTT protocol version (%d)", mqtt_res);
        goto done;
    }

    use_OS_cert = IsNullOrEmpty(g_moduleState.settings.mqttSettings.caFile);
    if (use_OS_cert)
    {
        mqtt_res = mosquitto_int_option(g_moduleState.mqttClient, MOSQ_OPT_TLS_USE_OS_CERTS, true);
        if (mqtt_res != MOSQ_ERR_SUCCESS)
        {
            Log_Error("Failed to set TLS use OS certs (%d)", mqtt_res);
            // TODO (nox-msft) - Handle all error codes.
            goto done;
        }
    }

    // TODO (nox-msft) - Set X.509 certificate here (if using certificate-based authentication)
    mqtt_res = mosquitto_tls_set(
        g_moduleState.mqttClient,
        g_moduleState.settings.mqttSettings.caFile,
        use_OS_cert ? "/etc/ssl/certs" : NULL,
        g_moduleState.settings.mqttSettings.certFile,
        g_moduleState.settings.mqttSettings.keyFile,
        NULL);
    if (mqtt_res != MOSQ_ERR_SUCCESS)
    {
        Log_Error("Failed to set TLS settings (%d)", mqtt_res);

        // Handle all error codes.
        switch (mqtt_res)
        {
        case MOSQ_ERR_INVAL:
            Log_Error("Invalid parameter");
            break;

        case MOSQ_ERR_NOMEM:
            Log_Error("Out of memory");
            break;
        }

        goto done;
    }

    /*callbacks */
    mosquitto_connect_v5_callback_set(g_moduleState.mqttClient, ADPS_OnConnect /*ADPS_OnConnect_WithSubscribe*/);
    mosquitto_disconnect_v5_callback_set(g_moduleState.mqttClient, ADPS_OnDisconnect);
    mosquitto_subscribe_v5_callback_set(g_moduleState.mqttClient, ADPS_OnSubscribe);
    mosquitto_publish_v5_callback_set(g_moduleState.mqttClient, ADPS_OnPublish);
    mosquitto_message_v5_callback_set(g_moduleState.mqttClient, ADPS_OnMessage);
    mosquitto_log_callback_set(g_moduleState.mqttClient, ADPS_OnLog);

    g_moduleState.initialized = true;
    result.ResultCode = ADUC_GeneralResult_Success;
    result.ExtendedResultCode = 0;

done:
    return result.ResultCode == ADUC_GeneralResult_Success ? 0 : -1;
}

/**
 * @brief Deinitialize the IoTHub Client module.
 */
bool ADUC_EnsureValidCommunicationChannel()
{
    if (!g_moduleState.initialized)
    {
        return false;
    }

    if (g_moduleState.connected)
    {
        return true;
    }

    // Connect to DPS
    int mqtt_res = mosquitto_connect(
        g_moduleState.mqttClient,
        g_moduleState.settings.mqttSettings.hostname,
        (int)g_moduleState.settings.mqttSettings.tcpPort,
        (int)g_moduleState.settings.mqttSettings.keepAliveInSeconds);

    if (mqtt_res != MOSQ_ERR_SUCCESS)
    {
        int errno_copy = errno;
        if (mqtt_res == MOSQ_ERR_ERRNO)
        {
            Log_Error("mosquitto_connect failed (%d) - %s", mqtt_res, strerror(errno_copy));
        }
        else
        {
            Log_Error("mosquitto_connect failed (%d)", mqtt_res);
        }
        // TODO (nox-msft): Determine whether to retry to bail according to error code
        /*
        	MOSQ_ERR_AUTH_CONTINUE = -4,
            MOSQ_ERR_NO_SUBSCRIBERS = -3,
            MOSQ_ERR_SUB_EXISTS = -2,
            MOSQ_ERR_CONN_PENDING = -1,
            MOSQ_ERR_SUCCESS = 0,
            MOSQ_ERR_NOMEM = 1,
            MOSQ_ERR_PROTOCOL = 2,
            MOSQ_ERR_INVAL = 3,
            MOSQ_ERR_NO_CONN = 4,
            MOSQ_ERR_CONN_REFUSED = 5,
            MOSQ_ERR_NOT_FOUND = 6,
            MOSQ_ERR_CONN_LOST = 7,
            MOSQ_ERR_TLS = 8,
            MOSQ_ERR_PAYLOAD_SIZE = 9,
            MOSQ_ERR_NOT_SUPPORTED = 10,
            MOSQ_ERR_AUTH = 11,
            MOSQ_ERR_ACL_DENIED = 12,
            MOSQ_ERR_UNKNOWN = 13,
            MOSQ_ERR_ERRNO = 14,
            MOSQ_ERR_EAI = 15,
            MOSQ_ERR_PROXY = 16,
            MOSQ_ERR_PLUGIN_DEFER = 17,
            MOSQ_ERR_MALFORMED_UTF8 = 18,
            MOSQ_ERR_KEEPALIVE = 19,
            MOSQ_ERR_LOOKUP = 20,
            MOSQ_ERR_MALFORMED_PACKET = 21,
            MOSQ_ERR_DUPLICATE_PROPERTY = 22,
            MOSQ_ERR_TLS_HANDSHAKE = 23,
            MOSQ_ERR_QOS_NOT_SUPPORTED = 24,
            MOSQ_ERR_OVERSIZE_PACKET = 25,
            MOSQ_ERR_OCSP = 26,
            MOSQ_ERR_TIMEOUT = 27,
            MOSQ_ERR_RETAIN_NOT_SUPPORTED = 28,
            MOSQ_ERR_TOPIC_ALIAS_INVALID = 29,
            MOSQ_ERR_ADMINISTRATIVE_ACTION = 30,
            MOSQ_ERR_ALREADY_EXISTS = 31,*/
        return false; // Connection failed
    }

    g_moduleState.connected = true;
    g_moduleState.lastConnectedTime = time(NULL);

    //mqtt_res = mosquitto_loop(g_moduleState.mqttClient, 100 /*timeout*/, 1 /*max_packets*/);
    mqtt_res = mosquitto_loop_start(g_moduleState.mqttClient);
    if (mqtt_res != MOSQ_ERR_SUCCESS)
    {
        Log_Error("Failed to start Mosquitto loop (%d)", mqtt_res);
        // TODO (nox-msft) - Handle all error codes.
        return false;
    }

    return true;
}

/**
 * @brief Ensure that the device is subscribed to DPS topics.
 * @return true if the device is subscribed to DPS topics.
 */
bool EnsureSubscribed()
{
    if (g_moduleState.subscribed)
    {
        return true;
    }

    int mqtt_ret = mosquitto_subscribe(g_moduleState.mqttClient, NULL, "$dps/registrations/res/#", 0);
    if (mqtt_ret != MOSQ_ERR_SUCCESS)
    {
        Log_Error("Failed to subscribe to DPS topics (%d)", mqtt_ret);
        // TODO (nox-msft) : Determine whether to retry or bail according to error code
        /*
            MOSQ_ERR_INVAL - if the input parameters were invalid.
            MOSQ_ERR_NOMEM - if an out of memory condition occurred.
            MOSQ_ERR_NO_CONN - if the client isn't connected to a broker.
            MOSQ_ERR_MALFORMED_UTF8 - if the topic is not valid UTF-8
            MOSQ_ERR_OVERSIZE_PACKET - if the resulting packet would be larger than
            supported by the broker.)
        */
    }
    else
    {
        g_moduleState.subscribed = true;
    }

    return g_moduleState.subscribed;
}

bool EnsureDeviceRegistered(/*const char* request_id, const char* connect_json*/)
{
    if (g_moduleState.registerState == ADPS_REGISTER_STATE_REGISTERED)
    {
        //     // TODO: check current registration status by sending a polling request
        //     char topic_name[256];
        //     snprintf(topic_name, sizeof(topic_name), "$dps/registrations/GET/iotdps-get-operationstatus/?$rid=%s&operationId=%s", request_id, operationId);
        // mosquitto_publish(mqtt_client, NULL, topic_name, 0, "", 0, false);
        //       snprintf(topic_name, sizeof(topic_name), "$dps/registrations/PUT/iotdps-register/?$rid=%s", request_id);
        //     mosquitto_publish(g_moduleState.mqttClient, NULL, topic_name, strlen(connect_json), connect_json, 0, false);

        return true;
    }

    if (g_moduleState.registerState == ADPS_REGISTER_STATE_REGISTERING)
    {
        // TODO (nox-msft) : Check for timeout
        return false;
    }

    if (g_moduleState.registerState == ADPS_REGISTER_STATE_FAILED)
    {
        // TODO (nox-msft) : determine whether to retry or bail
        return false;
    }

    if (g_moduleState.registerState == ADPS_REGISTER_STATE_NONE && g_moduleState.nextOperationTime > time(NULL))
    {
        char topic_name[256];
        char device_registration_json[1024];
        time_t request_id = GetTimeSinceEpochInSeconds();
        snprintf(
            topic_name, sizeof(topic_name), "$dps/registrations/PUT/iotdps-register/?$rid=adps_req_%ld", request_id);
        snprintf(
            device_registration_json,
            sizeof(device_registration_json),
            "{\"registrationId\":\"%s\"}",
            g_moduleState.settings.registrationId);
        int mqtt_ret = mosquitto_publish(
            g_moduleState.mqttClient,
            NULL,
            topic_name,
            strlen(device_registration_json),
            device_registration_json,
            0,
            false);

        if (mqtt_ret != MOSQ_ERR_SUCCESS)
        {
            Log_Error("Failed to publish registration request (%d)", mqtt_ret);
            // TODO (nox-msft) : determine whether to retry or bail
            return false;
        }
        else
        {
            g_moduleState.registerState = ADPS_REGISTER_STATE_REGISTERING;
        }
    }

    return false;
}

/**
 * @brief Deinitialize the IoTHub Client module.
 *
 * @param module The handle to the module. This is the same handle that was returned by the Create function.
 *
 * @return int 0 on success.
 */
int ADPS_MQTT_Client_Module_Deinitialize(ADUC_AGENT_MODULE_HANDLE module)
{
    Log_Info("Deinitialize");

    if (g_moduleState.mqttClient != NULL)
    {
        Log_Info("Disconnecting MQTT client");
        mosquitto_disconnect(g_moduleState.mqttClient);
        Log_Info("Destroying MQTT client");
        mosquitto_destroy(g_moduleState.mqttClient);
        g_moduleState.mqttClient = NULL;
    }

    mosquitto_lib_cleanup();
    ADUC_Logging_Uninit();
    return 0;
}

/**
 * @brief Create a Device Update Agent Module for IoT Hub PnP Client.
 *
 * @return ADUC_AGENT_MODULE_HANDLE The handle to the module.
 */
ADUC_AGENT_MODULE_HANDLE ADPS_MQTT_Client_Module_Create()
{
    ADUC_AGENT_MODULE_HANDLE handle = &g_moduleState;
    return handle;
}

/*
 * @brief Destroy the Device Update Agent Module for IoT Hub PnP Client.
 * @param handle The handle to the module. This is the same handle that was returned by the Create function.
 */
void ADPS_MQTT_Client_Module_Destroy(ADUC_AGENT_MODULE_HANDLE handle)
{
    IGNORED_PARAMETER(handle);
    Log_Info("DPS mqtt model destroy");
}

void OnCommandReceived(const char* commandName, const char* commandPayload, size_t payloadSize, void* context)
{
    IGNORED_PARAMETER(context);
    Log_Info("Received command %s with payload %s (size:%d)", commandName, commandPayload, payloadSize);
}

bool SubscribeToDSPModuleCommands()
{
    // TODO (nox-msft): subscribe to DPS module commands.
    // return InterModuleCommand_Subscribe("module.microsoft.dpsclient.ipcCommands", OnCommandReceived, NULL);
    return true;
}

void UnsubScribeToDSPModuleCommands()
{
    // TODO (nox-msft): unsubscribe to DPS module commands.
    // InterModuleCommand_Unsubscribe("module.microsoft.dpsclient.ipcCommands", OnCommandReceived, NULL);
}

bool IsConnectionResetRequested()
{
    // TODO (nox-msft): listen for a signal to reset the connection.
    return false;
}

/**
 * @brief Reset the connection.
 */
void ResetConnection()
{
    // TODO (nox-msft): ensure that we tear down the connection and re-establish it.
    g_moduleState.connected = g_moduleState.subscribed = false;
    g_moduleState.registerState = ADPS_REGISTER_STATE_NONE;
}

/**
 * @brief Perform the work for the extension. This must be a non-blocking operation.
 *
 * @return ADUC_Result
 */
int ADPS_MQTT_Client_Module_DoWork(ADUC_AGENT_MODULE_HANDLE handle)
{
    int mqtt_ret = MOSQ_ERR_UNKNOWN;

    if (!ADUC_EnsureValidCommunicationChannel())
    {
        return -1;
    }

    // // Executes mosquitto network loop. Setting time 0 value to immediately return when network connection is not available.
    // // NOTE: max_packets is unused.
    // int mqtt_ret = MOSQ_ERR_UNKNOWN;

    // mqtt_ret = mosquitto_loop(g_moduleState.mqttClient, 0 /*timeout*/, 1 /*max_packets*/);
    if (mqtt_ret != MOSQ_ERR_SUCCESS)
    {
        // Log_Error("Failed to execute mosquitto loop (%d)", mqtt_ret);
        // TODO (nox-msft) : determine whether to retry or bail
        return -1;
    }

    if (IsConnectionResetRequested())
    {
        Log_Info("Connection reset requested");
        ResetConnection();
    }

    if (!EnsureSubscribed())
    {
        return -1;
    }

    if (!EnsureDeviceRegistered())
    {
        return -1;
    }

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
int ADPS_MQTT_Client_Module_GetData(
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

ADUC_AGENT_MODULE_INTERFACE ADPS_MQTT_Client_ModuleInterface = {
    &g_moduleState,
    ADPS_MQTT_Client_Module_Create,
    ADPS_MQTT_Client_Module_Destroy,
    ADPS_MQTT_Client_Module_GetContractInfo,
    ADPS_MQTT_Client_Module_DoWork,
    ADPS_MQTT_Client_Module_Initialize,
    ADPS_MQTT_Client_Module_Deinitialize,
    ADPS_MQTT_Client_Module_GetData,
};
