/**
 * @file adu_agent_info_management.c
 * @brief Implementation for Device Update agent info management functions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adu_agent_info_management_internal.h"
#include "aduc/adu_mosquitto_utils.h"
#include "aduc/adu_mqtt_protocol.h"
#include "aduc/adu_types.h"
#include "aduc/c_utils.h" // IsNullOrEmpty
#include "aduc/config_utils.h"
#include "aduc/logging.h"
#include "aduc/retry_utils.h"
#include "aduc/string_c_utils.h" // ADUC_StringFormat
#include "aduc/types/update_content.h" // ADUCITF_FIELDNAME_DEVICEPROPERTIES_*
#include "du_agent_sdk/agent_module_interface.h"

#include <aducpal/time.h> // time_t

#include "parson.h" // JSON_Value, JSON_Object, etc.
#include <aducpal/stdlib.h>
#include <mosquitto.h> // mosquitto related functions
#include <mqtt_protocol.h> // mosquitto_property

#define DEFAULT_AINFO_REQ_RETRY_INTERVAL_SECONDS 60
#define SUBSCRIBE_TOPIC_TEMPLATE_AINFO_REQ ""

// Forward declarations.
static JSON_Value* PrepareAgentInformation();
static void UpdateNextAttemptTime(time_t time);
static ADUC_AGENT_INFO_MANAGEMENT_STATE* StateFromModule()

    // Declare the agent info management module functions.
    DECLARE_AGENT_MODULE_PRIVATE(ADUC_Agent_Info_Management)

    // Implement all functions declared by DECLARE_AGENT_MODULE_PRIVATE macro above.

    /**
 * @brief Create the agent info management module instance
 */
    ADUC_AGENT_MODULE_HANDLE ADUC_Agent_Info_Management_Create()
{
    ADUC_AGENT_MODULE_HANDLE handle = NULL;

    // Create the agent info management module state.
    ADUC_AGENT_INFO_MANAGEMENT_STATE* state = malloc(sizeof(*state));
    if (state == NULL)
    {
        Log_Error("Failed to allocate memory for the agent info management state.");
        return NULL;
    }
    memset(state, 0, sizeof(*state));

    ADUC_AGENT_MODULE_INTERFACE* agentInfoModuleInterface = malloc(sizeof(*agentInfoModuleInterface));
    if (agentInfoModuleInterface == NULL)
    {
        Log_Error("Failed to allocate memory for the agent info management module interface.");
        free(state);
        return NULL;
    }
    memset(agentInfoModuleInterface, 0, sizeof(*agentInfoModuleInterface));

    agentInfoModuleInterface->moduleHandle = state;
    agentInfoModuleInterface->Initialize = ADUC_Agent_Info_Management_Initialize;
    agentInfoModuleInterface->deinitializeModule = ADUC_Agent_Info_Management_Deinitialize;
    agentInfoModuleInterface->doWork = ADUC_Agent_Info_Management_DoWork;
    agentInfoModuleInterface->getData = ADUC_Agent_Info_Management_GetData;
    agentInfoModuleInterface->setData = ADUC_Agent_Info_Management_SetData;

    handle = agentInfoModuleInterface;

done:
    if (handle == NULL)
    {
        free(state);
        free(agentInfoModuleInterface);
    }

    return handle;
}

/**
 * @brief Initialize the agent info management.
 */
int ADUC_Agent_Info_Management_Initialize(
    ADUC_AGENT_MODULE_HANDLE agentInfoModule, void* initData /* ADUC_AGENT_INFO_MANAGEMENT_INIT_DATA* */)
{
    return -1;
}

/**
 * @brief Deinitialize the agent info management.
 */
void ADUC_Agent_Info_Management_Deinitialize(ADUC_AGENT_MODULE_HANDLE agentInfoModule)
{
}

static ADUC_AGENT_INFO_MANAGEMENT_STATE s_agentInfoMgrState = { 0 };

#define AGENT_INFO_FIELD_NAME_AIV "aiv"
#define AGENT_INFO_FIELD_NAME_ADU_PROTOCOL "aduProtocol"
#define AGENT_INFO_FIELD_NAME_ADU_PROTOCOL_NAME "name"
#define AGENT_INFO_FIELD_NAME_ADU_PROTOCOL_VERSION "version"
#define AGENT_INFO_FIELD_NAME_COMPAT_PROPERTIES "compatProperties"
#define AGENT_INFO_PROTOCOL_NAME "adu"
#define AGENT_INFO_PROTOCOL_VERSION 1

/**
 * @brief This helper function will prepare a JSON data object for the 'ainfo_req' message.
*/
JSON_Value* PrepareAgentInformation()
{
    const ADUC_ConfigInfo* config = NULL;
    const ADUC_AgentInfo* agent = NULL;
    JSON_Value* agentInfoValue = NULL;
    JSON_Object* agentInfoObj = NULL;
    JSON_Value* aduProtocolValue = NULL;
    JSON_Object* aduProtocolObj = NULL;
    JSON_Value* compatPropertiesValue = NULL;
    JSON_Object* compatPropertiesObj = NULL;
    JSON_Object* additional_properties = NULL;

    JSON_Status jsonStatus = JSONFailure;
    bool success = false;

    config = ADUC_ConfigInfo_GetInstance();
    if (config == NULL)
    {
        Log_Error("Failed to get the configuration info.");
        goto done;
    }

    agent = ADUC_ConfigInfo_GetAgent(config, 0);
    if (agent == NULL)
    {
        Log_Error("Failed to get the agent info.");
        goto done;
    }

    agentInfoValue = json_value_init_object();
    agentInfoObj = json_value_get_object(agentInfoValue);
    if (agentInfoObj == NULL)
    {
        Log_Error("Cannot allocate memory for agent information.");
        goto done;
    }

    time_t nowTime = ADUC_GetTimeSinceEpochInSeconds();

    // Add agent information version.
    jsonStatus = json_object_set_number(agentInfoObj, AGENT_INFO_FIELD_NAME_AIV, nowTime);
    if (jsonStatus != JSONSuccess)
    {
        Log_Error("Could not serialize JSON field: aiv, value: %d", AGENT_INFO_FIELD_NAME_AIV, nowTime);
        goto done;
    }

    // Add ADU protocol name and version.
    aduProtocolValue = json_value_init_object();
    aduProtocolObj = json_value_get_object(aduProtocolValue);
    if (aduProtocolObj == NULL)
    {
        Log_Error("Cannot allocate memory for aduProtocol.");
        goto done;
    }
    jsonStatus = json_object_set_value(agentInfoObj, AGENT_INFO_FIELD_NAME_ADU_PROTOCOL, aduProtocolValue);
    if (jsonStatus != JSONSuccess)
    {
        Log_Error(
            "Could not serialize JSON field: %s value: %p", AGENT_INFO_FIELD_NAME_ADU_PROTOCOL, aduProtocolValue);
        goto done;
    }
    aduProtocolValue = NULL; // Ownership transferred to agentInfoObj.

    jsonStatus =
        json_object_set_string(aduProtocolObj, AGENT_INFO_FIELD_NAME_ADU_PROTOCOL_NAME, AGENT_INFO_PROTOCOL_NAME);
    if (jsonStatus != JSONSuccess)
    {
        Log_Error(
            "Could not serialize JSON field: %s value: %s",
            AGENT_INFO_FIELD_NAME_ADU_PROTOCOL_NAME,
            AGENT_INFO_PROTOCOL_NAME);
        goto done;
    }

    jsonStatus = json_object_set_number(
        aduProtocolObj, AGENT_INFO_FIELD_NAME_ADU_PROTOCOL_VERSION, AGENT_INFO_PROTOCOL_VERSION);
    if (jsonStatus != JSONSuccess)
    {
        Log_Error(
            "Could not serialize JSON field: %s value: %d",
            AGENT_INFO_FIELD_NAME_ADU_PROTOCOL_VERSION,
            AGENT_INFO_PROTOCOL_VERSION);
        goto done;
    }

    // Add 'compatProperties' object.
    compatPropertiesValue = json_value_init_object();
    compatPropertiesObj = json_value_get_object(compatPropertiesValue);
    if (compatPropertiesObj == NULL)
    {
        Log_Error("Cannot allocate memory for compatProperties.");
        goto done;
    }

    jsonStatus = json_object_set_value(agentInfoObj, AGENT_INFO_FIELD_NAME_COMPAT_PROPERTIES, compatPropertiesValue);
    if (jsonStatus != JSONSuccess)
    {
        Log_Error(
            "Could not serialize JSON field: %s value: %p",
            AGENT_INFO_FIELD_NAME_COMPAT_PROPERTIES,
            compatPropertiesValue);
        goto done;
    }
    compatPropertiesValue = NULL; // Ownership transferred to agentInfoObj.

    // Add manufacturer and model.
    jsonStatus = json_object_set_string(
        compatPropertiesObj, ADUCITF_FIELDNAME_DEVICEPROPERTIES_MANUFACTURER, agent->manufacturer);

    if (jsonStatus != JSONSuccess)
    {
        Log_Error(
            "Could not serialize JSON field: %s value: %s",
            ADUCITF_FIELDNAME_DEVICEPROPERTIES_MANUFACTURER,
            agent->manufacturer);
        goto done;
    }

    jsonStatus = json_object_set_string(compatPropertiesObj, ADUCITF_FIELDNAME_DEVICEPROPERTIES_MODEL, agent->model);

    if (jsonStatus != JSONSuccess)
    {
        Log_Error(
            "Could not serialize JSON field: %s value: %s",
            ADUCITF_FIELDNAME_DEVICEPROPERTIES_MODEL,
            agent->manufacturer);
        goto done;
    }

    // Add additional properties.
    additional_properties = agent->additionalDeviceProperties;
    if (additional_properties == NULL)
    {
        // No additional properties is set in the configuration file
        success = true;
        goto done;
    }

    size_t propertiesCount = json_object_get_count(additional_properties);
    for (size_t i = 0; i < propertiesCount; i++)
    {
        const char* name = json_object_get_name(additional_properties, i);
        const char* val = json_value_get_string(json_object_get_value_at(additional_properties, i));

        if ((name == NULL) || (val == NULL))
        {
            Log_Error(
                "Error retrieving the additional device properties name and/or value at element at index=%zu", i);
            goto done;
        }

        jsonStatus = json_object_set_string(compatPropertiesObj, name, val);
        if (jsonStatus != JSONSuccess)
        {
            Log_Error("Could not serialize JSON field: %s value: %s", name, val);
            goto done;
        }
    }

    success = true;

done:
    if (!success)
    {
        Log_Error("Failed to get prepare an agent information object.");
        json_value_free(compatPropertiesValue) json_value_free(aduProtocolValue);
        json_value_free(agentInfoValue);
    }

    return agentInfoValue;
}

const char* GetLastMessageCorrelationId()
{
    return s_agentInfoMgrState.ainfo_req_CorelationId;
}

/**
 * @brief This function handles the 'ainfo_resp' message from the broker. The function will create a copy of the
 * message and process in the net do_work cycle.
*/
void OnMessage_ainfo_resp(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props)
{
    Log_Info("RCV msg: id=%d, topic=%s, payload=%s", msg->mid, msg->topic, (char*)msg->payload);
    JSON_Value* responseJson = NULL;
    JSON_Object* responseObj = NULL;
    int resultCode = 0;

    ADUC_AGENT_INFO_MANAGEMENT_STATE* state = (ADUC_AGENT_INFO_MANAGEMENT_STATE*)obj;
    bool matched =
        CheckMessageTypeAndCorrelationId(msg, props, ADUC_MQTT_MESSAGE_TYPE_AINFO_RESP, GetLastMessageCorrelationId());
    if (!matched)
    {
        Log_Info("Not interested in this message.");
        goto done;
    }

    // Parse message payload.
    responseJson = json_parse_string((char*)msg->payload);
    if (responseJson == NULL)
    {
        Log_Error("Failed to parse the response message payload.");
        goto done;
    }

    responseObj = json_value_get_object(responseJson);
    if (responseObj == NULL)
    {
        Log_Error("Failed to get the response message object.");
        goto done;
    }

    json_object_dotget_number(responseObj, ADUC_AGENT_INFO_RESPONSE_FIELD_RESULT_CODE, &resultCode);

    // Handle ADU_RESPONSE_MESSAGE_RESULT_CODE
    // Classify the result code in to success, transient, non-recoverable and other.
    /*
        ADU_RESPONSE_MESSAGE_RESULT_CODE_SUCCESS = 0,           /**< Operation was successful. */
    ADU_RESPONSE_MESSAGE_RESULT_CODE_BAD_REQUEST = 1, /**< The request was invalid or cannot be served. */
        ADU_RESPONSE_MESSAGE_RESULT_CODE_BUSY = 2, /**< The server is busy and cannot process the request. */
        ADU_RESPONSE_MESSAGE_RESULT_CODE_CONFLICT =
            3, /**< There is a conflict with the current state of the system. */
        ADU_RESPONSE_MESSAGE_RESULT_CODE_SERVER_ERROR = 4, /**< The server encountered an internal error. */
        ADU_RESPONSE_MESSAGE_RESULT_CODE_AGENT_NOT_ENROLLED =
            5 * / Log_Info("Received ainfo_resp message with result code: %d", resultCode);
    switch (resultCode)
    {
    case ADUC_AGENT_INFO_RESPONSE_RESULT_CODE_SUCCESS:
        break;

    case ADUC_AGENT_INFO_RESPONSE_RESULT_CODE_CONFLICT:
        // The agent info is older than what the service has.
        break;

    case ADUC_AGENT_INFO_RESPONSE_RESULT_CODE_AGENT_NOT_ENROLLED:
        // Need to update the enrollment state.
        ADUC_SendAgentCommand(ADUC_AGENT_COMMAND_RESET_ENROLLMENT_STATE, NULL);
        break;

    case ADUC_AGENT_INFO_RESPONSE_RESULT_CODE_BAD_REQUEST:
        Log_Info("Received ainfo_resp message with result code: %d", resultCode);
        break;
    case ADUC_AGENT_INFO_RESPONSE_RESULT_CODE_BUSY:
        Log_Info("Received ainfo_resp message with result code: %d", resultCode);
        break;
    case ADUC_AGENT_INFO_RESPONSE_RESULT_CODE_SERVER_ERROR:
        Log_Info("Received ainfo_resp message with result code: %d", resultCode);
        break;

        if (resultCode != 200 && resultCode != 204)
        {
            Log_Error("Failed to get the result code from the response message.");
            goto done;
        }
        {
            /
        }

        // Check that the message's correlation ID matches the one we sent.
        char* correlationId = NULL;
        if (ADUC_MQTT_GetCorrelationId(msg, &correlationId) != ADUC_MQTT_OK)
        {
            Log_Error("OnMessage_ainfo_resp: Failed to get the correlation ID.");
            goto done;
        }

    done:
        free(correlationId);
    }

    void OnPublished_ainfo_req(
        struct mosquitto * mosq, void* obj, int mid, int reason_code, const mosquitto_property* props)
    {
        Log_Info("on_publish: Message with mid %d has been published.", mid);
    }

    /**
 * @brief Update the next attempt time based on the error code.
 */
    static void UpdateNextAttemptTime(int errorCode)
    {
        // TODO: use retry_utils to update the next attempt time based on the given error code.
        s_agentInfoMgrState.ainfo_req_NextAttemptTime += DEFAULT_AINFO_REQ_RETRY_INTERVAL_SECONDS;
    }

    /**
 * @brief Initializes the agent info management workflow.
 */
    bool ADUC_Agent_Info_Management_Initialize()
    {
        s_agentInfoMgrState.agentInfoValue = PrepareAgentInformation();
        if (s_agentInfoMgrState.agentInfoValue == NULL)
        {
            UpdateNextAttemptTime(0 /* error code */);
            Log_Error("Failed to prepare the agent information.");
            return false;
        }

        s_agentInfoMgrState.workflowState = ADUC_AGENT_INFO_WORKFLOW_STATE_INITIALIZED;
        return true;
    }

    static void FreeAgentInfoManagementState()
    {
        free(s_agentInfoMgrState.publishTopic);
        s_agentInfoMgrState.publishTopic = NULL;
        free(s_agentInfoMgrState.subscribeTopic);
        s_agentInfoMgrState.subscribeTopic = NULL;
        free(s_agentInfoMgrState.ainfo_req_CorelationId);
        s_agentInfoMgrState.ainfo_req_CorelationId = NULL;
        free(s_agentInfoMgrState.ainfo_resp_Content);
        s_agentInfoMgrState.ainfo_resp_Content = NULL;
    }

    /**
 * @brief Deinitializes the agent info management.
 */
    void ADUC_Agent_Info_Management_Deinitialize()
    {
        FreeAgentInfoManagementState();
    }

    /**
 * @brief This function ensures that the 'ainfo_req' topic template is initialized and the topic is subscribed.
 */
    static bool EnsureResponseTopicSubscribed()
    {
        bool success = false;
        if (s_agentInfoMgrState.workflowState >= ADUC_AGENT_INFO_WORKFLOW_STATE_SUBSCRIBED)
        {
            success = true;
            goto done;
        }

        time_t nowTime = ADUC_GetTimeSinceEpochInSeconds();
        if (nowTime < s_agentInfoMgrState.ainfo_req_NextAttemptTime)
        {
            // Wait for the next attempt time.
            goto done;
        }

        if (s_agentInfoMgrState.workflowState == ADUC_AGENT_INFO_WORKFLOW_STATE_INITIALIZED)
        {
            if (IsNullOrEmpty(s_agentInfoMgrState.subscribeTopic))
            {
                const char* duInstance = ADUC_StateStore_GetDeviceUpdateServiceInstance();
                if (IsNullOrEmpty(duInstance))
                {
                    Log_Error("Invalid state. DU instance is NULL.");
                    UpdateNextAttemptTime(0 /* error code */);
                    goto done;
                }

                const char* externalDeviceId = ADUC_StateStore_GetExternalDeviceId();
                s_agentInfoMgrState.subscribeTopic =
                    ADUC_StringFormat(SUBSCRIBE_TOPIC_TEMPLATE_ADU_OTO_WITH_DU_INSTANCE, externalDeviceId, duInstance);

                if (IsNullOrEmpty(s_agentInfoMgrState.subscribeTopic))
                {
                    Log_Error("Failed to format the subscribe topic.");
                    UpdateNextAttemptTime(0 /* error code */);
                    goto done;
                }
            }

            ADUC_StateStore_GetCommunicationChannelHandle(
                &s_agentInfoMgrState.publishTopic, &s_agentInfoMgrState.publishTopic);

            if (ADUC_MQTT_Subscribe(s_agentInfoMgrState.subscribeTopic, OnMessage_ainfo_resp))

                // TODO: subscribe to response topic.
                // if (s_agentInfoMgrState.subscribeTopic, OnMessage_ainfo_resp))
                // {
                //     Log_Error("Failed to subscribe to the topic: %s", s_agentInfoMgrState.subscribeTopic);
                //     return false;
                // }

                s_agentInfoMgrState.workflowState = ADUC_AGENT_INFO_WORKFLOW_STATE_SUBSCRIBING;
        }

        if (s_agentInfoMgrState.workflowState == ADUC_AGENT_INFO_WORKFLOW_STATE_SUBSCRIBING)
        {
        }

    done:
        return success;
    }

    /**
 * @brief This function waits for the client to verify that it is enrolled with the Device Update service.
 * Once the client is enrolled, it will publish an 'ainfo_req' message to the service.
 * The message will contains the following information:
 * - aiv: The agent information version. This is a timestamp in seconds.
 * - aduProtocolVersion: The ADU protocol version. This is a name and version number.
 * - compatProperties: A list of properties that the client supports.
 *
 * Once the message is published, the function will wait for the acknowledgement message ('ainfo_resp').
 * The acknowledgement message will contain the update information.
 *
 * This function will validate the update data and hand it off to the agent workflow util.
 *
 * @return true if the there's no error, false otherwise.
 *
 */
    bool ADUC_Agent_Info_Management_DoWork()
    {
        time_t nowTime = ADUC_GetTimeSinceEpochInSeconds();
        if (!ADUC_StateStore_GetIsDeviceEnrolled())
        {
            if (nowTime > s_agentInfoMgrState.ainfo_req_NextAttemptTime)
            {
                Log_Info("Device is not enrolled. Waiting for enrollment to complete.");
                UpdateNextAttemptTime(0 /* error code */);
            }
            return false;
        }

        // TODO (nox-msft)
        // if (!EnsureResponseTopicSubscribed())
        //{
        //    return false;
        // }

        // TODO (nox-msft)
        //if (!EnsureAgentInfoMessagePublished())
        //{
        //    return false;
        // }

        // TODO (nox-msft)
        // if (!EnsureAgentInfoAcknowledged())
        // {
        //     return false;
        // }

        return true;
    }
