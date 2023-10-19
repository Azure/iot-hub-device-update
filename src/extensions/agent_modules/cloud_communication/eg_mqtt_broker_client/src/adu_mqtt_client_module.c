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
#include "aduc/agent_state_store.h"
#include "aduc/config_utils.h"
#include "aduc/eg_mqtt_broker_client.h"
#include "aduc/logging.h"
#include "aduc/retry_utils.h"
#include "aduc/string_c_utils.h"
#include "du_agent_sdk/agent_module_interface.h"

#include "aducpal/time.h" // time_t
#include "errno.h"
#include "string.h"
#include <mosquitto.h> // mosquitto related functions
#include <mqtt_protocol.h> // mosquitto_property

// Forward declarations
int ADUC_MQTT_Client_Module_Initialize_DoWork(ADUC_MQTT_CLIENT_MODULE_STATE* state);
void ADUC_MQTT_Client_Module_Destroy(ADUC_AGENT_MODULE_HANDLE handle);
int ADUC_MQTT_Client_Module_DoWork(ADUC_AGENT_MODULE_HANDLE handle);

#define DEFAULT_OPERATION_INTERVAL_SECONDS (5)

static ADUC_MQTT_CLIENT_MODULE_STATE* ModuleStateFromModuleHandle(ADUC_AGENT_MODULE_HANDLE handle)
{
    if ((handle != NULL) && (((ADUC_AGENT_MODULE_INTERFACE*)handle)->moduleData != NULL))
    {
        return (ADUC_MQTT_CLIENT_MODULE_STATE*)((ADUC_AGENT_MODULE_INTERFACE*)handle)->moduleData;
    }
    return NULL;
}

/**
 * @brief Deinitialize the MQTT topics used for Azure Device Update communication.
 * @return `true` if MQTT topics were successfully de-initialized; otherwise, `false`.
*/
static void DeinitMqttTopics(ADUC_MQTT_CLIENT_MODULE_STATE* moduleState)
{
    free(moduleState->mqtt_topic_service2agent);
    moduleState->mqtt_topic_service2agent = NULL;
    free(moduleState->mqtt_topic_agent2service);
    moduleState->mqtt_topic_agent2service = NULL;
}

/**
 * @brief Initializes MQTT topics for Azure Device Update communication.
 *
 * This function prepares MQTT topics for communication between an Azure Device Update client and server.
 * It constructs topics based on templates and the device ID, ensuring that the required topics are ready for use.
 *
 * @return `true` if MQTT topics were successfully prepared; otherwise, `false`.
 */
bool InitMqttTopics(ADUC_MQTT_CLIENT_MODULE_STATE* moduleState)
{
    bool result = false;
    const char* deviceId;

    if (!IsNullOrEmpty(moduleState->mqtt_topic_service2agent) && !IsNullOrEmpty(moduleState->mqtt_topic_agent2service))
    {
        return true;
    }

    deviceId = ADUC_StateStore_GetExternalDeviceId();

    if (IsNullOrEmpty(deviceId))
    {
        Log_Error("Invalid device id.");
        goto done;
    }

    // Create string for agent to service topic, from PUBLISH_TOPIC_TEMPLATE_ADU_OTO and the device id.
    moduleState->mqtt_topic_agent2service = ADUC_StringFormat(PUBLISH_TOPIC_TEMPLATE_ADU_OTO, deviceId);
    if (moduleState->mqtt_topic_agent2service == NULL)
    {
        goto done;
    }

    // Create string for service to agent topic, from SUBSCRIBE_TOPIC_TEMPLATE_ADU_OTO and the device id.
    moduleState->mqtt_topic_service2agent = ADUC_StringFormat(SUBSCRIBE_TOPIC_TEMPLATE_ADU_OTO, deviceId);
    if (moduleState->mqtt_topic_service2agent == NULL)
    {
        goto done;
    }

    result = true;

done:
    if (!result)
    {
        DeinitMqttTopics(moduleState);
    }
    return result;
}

/**
 * @brief Free resources allocated for the module state.
 * @param state The module state to free.
 */
void ADUC_MQTT_CLIENT_MODULE_STATE_Free(ADUC_MQTT_CLIENT_MODULE_STATE* state)
{
    DeinitMqttTopics(state);
    FreeMqttBrokerSettings(&state->mqttSettings);
}

/**
 * @brief The contract info for the module.
 */
ADUC_AGENT_CONTRACT_INFO g_egMqttClientContractInfo = {
    "Microsoft", "Event Grid MQTT Client Module", 1, "Microsoft/AzureEGMQTTClientModule:1"
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

/**
 * @brief This call back is called when a message is received from the MQTT broker.
 * @param mosq The mosquitto instance invoking the callback.
 * @param obj The user data provided in mosquitto_new. ()
 */
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

    // if (strcmp(msg_type, "ainfo_resp") == 0)
    // {
    //     OnMessage_ainfo_resp(mosq, obj, msg, props);
    //     goto done;
    // }

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

static char* s_subscriptionTopics[1];

/**
 * @brief Get the MQTT subscription topics for the MQTT client.
 *
 * This function retrieves the MQTT subscription topics used by the MQTT client for message reception.
 * It initializes the MQTT topics if not already initialized and returns a pointer to the array of topics
 * along with the count of topics.
 *
 * @param count A pointer to an integer where the count of subscription topics will be stored.
 * @param topics A pointer to a pointer to char arrays (strings) where the subscription topics will be stored.
 *
 * @return true if the operation was successful; false if any of the input parameters are NULL or if initialization fails.
 *
 * @note The caller MUST NOTE freeing the memory allocated for 'topics' and its contents.
 */
bool ADUC_MQTT_Client_GetSubscriptionTopics(void* obj, int* count, char*** topics)
{
    if (count == NULL || topics == NULL || obj == NULL)
    {
        Log_Error("NULL parameter. (count=%p, topics=%p, obj=%p)", count, topics, obj);
        return false;
    }

    ADUC_MQTT_CLIENT_MODULE_STATE* moduleState = (ADUC_MQTT_CLIENT_MODULE_STATE*)obj;

    if (!InitMqttTopics(moduleState))
    {
        Log_Error("Failed to initialize MQTT topics");
        return false;
    }

    // Always have only 1 topic to subscribe.
    *count = 1;

    s_subscriptionTopics[0] = moduleState->mqtt_topic_service2agent;
    *topics = s_subscriptionTopics;
    return true;
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

/* Callback called when the broker sends a CONNACK message in response to a
 * connection. */
void ADUC_MQTT_Client_OnConnect(
    struct mosquitto* mosq, void* obj, int reason_code, int flags, const mosquitto_property* props)
{
    if (reason_code == 0)
    {
        Log_Info("on_connect: Connection succeeded.");
    }
}

/* Callback called when the broker sends a SUBACK in response to a SUBSCRIBE. */
void ADUC_MQTT_Client_OnSubscribe(
    struct mosquitto* mosq, void* obj, int mid, int qos_count, const int* granted_qos, const mosquitto_property* props)
{
    ADUC_MQTT_CLIENT_MODULE_STATE* moduleState = (ADUC_MQTT_CLIENT_MODULE_STATE*)obj;
    if (moduleState->enrollmentState <= ADU_ENROLLMENT_STATE_UNKNOWN)
    {
        Log_Info("on_subscribe: Subscribing succeeded.");
        moduleState->enrollmentState = ADU_ENROLLMENT_STATE_SUBSCRIBED;
    }
}

/**
 * @brief Callbacks for various event from the communication channel.
 */
ADUC_MQTT_CALLBACKS s_duClientCommChannelCallbacks = {
    ADUC_MQTT_Client_OnConnect /*on_connect*/,
    NULL /*on_disconnect*/,
    ADUC_MQTT_Client_GetSubscriptionTopics /* get_subscription_topics */,
    ADUC_MQTT_Client_OnSubscribe /*on_subscribe*/,
    NULL /*on_unsubscribe*/,
    ADUC_MQTT_Client_OnPublish,
    ADUC_MQTT_Client_OnMessage,
    NULL /*on_log*/
};

/**
 * @brief Initialize the Device Update MQTT client module.
 */
int ADUC_MQTT_Client_Module_Initialize(ADUC_AGENT_MODULE_HANDLE handle, void* moduleInitData)
{
    ADUC_MQTT_CLIENT_MODULE_STATE* moduleState = ModuleStateFromModuleHandle(handle);
    if (moduleState == NULL)
    {
        Log_Error("handle is NULL");
        return -1;
    }

    moduleState->moduleInitData = moduleInitData;

    // Need to set ret and goto done after this to ensure proper shutdown and deinitialization.
    // TODO (nox-msft) - set log level according to global config.
    ADUC_Logging_Init(0, "du-mqtt-client-module");

    // Read DU Service MQTT client settings.
    bool success = ReadMqttBrokerSettings(&moduleState->mqttSettings);
    if (!success)
    {
        Log_Error("Failed to read Device Update MQTT client settings");
        //moduleState->initializeState = ADU_MQTT_CLIENT_MODULE_INITIALIZE_STATE_FAILED;
        goto done;
    }

    // Initialize communication channel.
    if (moduleState->commChannelModule != NULL)
    {
        ADUC_COMMUNICATION_CHANNEL_INIT_DATA commInitData = {
            ADUC_DU_SERVICE_COMMUNICATION_CHANNEL_ID,
            moduleState,
            &moduleState->mqttSettings,
            &s_duClientCommChannelCallbacks,
            NULL /* key file password callback */,
            NULL /* connection retry callback */
        };

        moduleState->commChannelModule->initializeModule(moduleState->commChannelModule, &commInitData);
    }

    if (moduleState->enrollmentModule == NULL)
    {
        Log_Error("Enrollment module is not initialized.");
    }
    else
    {
        moduleState->enrollmentModule->initializeModule(moduleState->enrollmentModule, moduleState->commChannelModule);
    }

    if (!success)
    {
        Log_Error("Failed to initialize the enrollment management");
        goto done;
    }

    // TODO: Enable agent info reporting when DU service support it.
    // success = ADUC_Agent_Info_Management_Initialize();
    // if (!success)
    // {
    //     Log_Error("Failed to initialize the agent info management");
    //     goto done;
    // }

    // TODO: Enable update management when DU service support it.
    // success = ADUC_Updates_Management_Initialize();
    // if (!success)
    // {
    //     Log_Error("Failed to initialize the updates management");
    //     goto done;
    // }

    success = true;
done:
    return success ? 0 : -1;
}

/**
 * @brief Deinitialize the IoTHub Client module.
 *
 * @param module The handle to the module. This is the same handle that was returned by the Create function.
 *
 * @return int 0 on success.
 */
int ADUC_MQTT_Client_Module_Deinitialize(ADUC_AGENT_MODULE_HANDLE handle)
{
    Log_Info("Deinitialize");
    ADUC_MQTT_CLIENT_MODULE_STATE* moduleState = ModuleStateFromModuleHandle(handle);

    if (moduleState->commChannelModule != NULL)
    {
        ADUC_AGENT_MODULE_INTERFACE* commInterface = (ADUC_AGENT_MODULE_INTERFACE*)moduleState->commChannelModule;
        commInterface->deinitializeModule(moduleState->commChannelModule);
    }

    ADUC_MQTT_CLIENT_MODULE_STATE_Free(moduleState);

    ADUC_Logging_Uninit();
    return 0;
}

/*
 * @brief Destroy the Device Update MQTT client module.
 * @param handle The handle to the module. This is the same handle that was returned by the Create function.
 */
void ADUC_MQTT_Client_Module_Destroy(ADUC_AGENT_MODULE_HANDLE handle)
{
    Log_Info("Destroying module:%p", handle);
    ADUC_AGENT_MODULE_INTERFACE* interface = AgentModuleInterfaceFromHandle(handle);
    interface->deinitializeModule(handle);
    free(interface);
}

/**
 * @brief Create a Device Update MQTT Client module.
 *
 * @return ADUC_AGENT_MODULE_HANDLE The handle to the module.
 */
ADUC_AGENT_MODULE_HANDLE ADUC_MQTT_Client_Module_Create()
{
    ADUC_AGENT_MODULE_HANDLE handle = NULL;

    ADUC_MQTT_CLIENT_MODULE_STATE* moduleState = malloc(sizeof(ADUC_MQTT_CLIENT_MODULE_STATE));
    if (moduleState == NULL)
    {
        Log_Error("Failed to allocate memory for module state");
        goto done;
    }
    memset(moduleState, 0, sizeof(ADUC_MQTT_CLIENT_MODULE_STATE));

    ADUC_AGENT_MODULE_INTERFACE* moduleInterface = malloc(sizeof(ADUC_AGENT_MODULE_INTERFACE));
    if (moduleInterface == NULL)
    {
        Log_Error("Failed to allocate memory for module interface");
        goto done;
    }

    memset(moduleInterface, 0, sizeof(ADUC_AGENT_MODULE_INTERFACE));
    moduleInterface->moduleData = moduleState;
    moduleInterface->getContractInfo = ADUC_MQTT_Client_Module_GetContractInfo;
    moduleInterface->initializeModule = ADUC_MQTT_Client_Module_Initialize;
    moduleInterface->doWork = ADUC_MQTT_Client_Module_DoWork;
    moduleInterface->deinitializeModule = ADUC_MQTT_Client_Module_Deinitialize;
    moduleInterface->destroy = ADUC_MQTT_Client_Module_Destroy;

    // Create communication channel.
    moduleState->commChannelModule = ADUC_Communication_Channel_Create();
    if (moduleState->commChannelModule == NULL)
    {
        Log_Error("Failed to create the communication channel");
        goto done;
    }

    // Create enrollment module.
    moduleState->enrollmentModule = ADUC_Enrollment_Management_Create();
    {
        if (moduleState->enrollmentModule == NULL)
        {
            Log_Error("Failed to create the enrollment management");
            goto done;
        }
    }

    handle = moduleInterface;

done:
    if (handle == NULL)
    {
        free(moduleState);
        free(moduleInterface);
    }
    return handle;
}

/**
 * @brief Perform the work for the extension. This must be a non-blocking operation.
 *
 * @return ADUC_Result
 */
int ADUC_MQTT_Client_Module_DoWork(ADUC_AGENT_MODULE_HANDLE handle)
{
    ADUC_MQTT_CLIENT_MODULE_STATE* moduleState = ModuleStateFromModuleHandle(handle);
    if (moduleState == NULL)
    {
        Log_Error("moduleState is NULL");
        return -1;
    }

    //success = enro
    time_t nowTime = ADUC_GetTimeSinceEpochInSeconds();
    // if (moduleState->mqttSettings.hostnameSource == ADUC_MQTT_HOSTNAME_SOURCE_DPS
    //     && moduleState->initializeState == ADU_MQTT_CLIENT_MODULE_INITIALIZE_STATE_PARTIAL)
    // {
    //     if (moduleState->nextOperationTime > nowTime)
    //     {
    //         goto done;
    //     }

    //     if (ADUC_MQTT_Client_Module_Initialize_DoWork(moduleState) != 0)
    //     {
    //         moduleState->nextOperationTime = nowTime + DEFAULT_OPERATION_INTERVAL_SECONDS;
    //         goto done;
    //     }
    //     else
    //     {
    //         moduleState->nextOperationTime = 0;
    //     }
    // }

    ADUC_AGENT_MODULE_INTERFACE* commInterface = (ADUC_AGENT_MODULE_INTERFACE*)moduleState->commChannelModule;
    if (commInterface == NULL)
    {
        Log_Error("commInterface is NULL");
        moduleState->nextOperationTime = nowTime + DEFAULT_OPERATION_INTERVAL_SECONDS;
        goto done;
    }

    commInterface->doWork(moduleState->commChannelModule);

    if (!ADUC_Communication_Channel_IsConnected(moduleState->commChannelModule))
    {
        // TODO: (nox-msft) - use retry utils.
        moduleState->nextOperationTime = nowTime + DEFAULT_OPERATION_INTERVAL_SECONDS;
        goto done;
    }

    if (moduleState->enrollmentModule != NULL)
    {
        moduleState->enrollmentModule->doWork(moduleState->enrollmentModule);
    }

    if (ADUC_StateStore_GetIsDeviceEnrolled())
    {
        // TODO: Enable this when DU Service implemented support for these functionalities.
        // ADUC_Agent_Info_Management_DoWork();
        // ADUC_Updates_Management_DoWork();
    }

done:
    return 0;
}
