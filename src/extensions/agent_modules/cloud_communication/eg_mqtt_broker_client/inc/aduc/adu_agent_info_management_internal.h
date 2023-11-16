/**
 * @file adu_agent_info_management_internal.h
 * @brief Header file for Device Update agent info management functions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef __ADUC_AGENT_INFO_MANAGEMENT_INTERNAL_H__
#define __ADUC_AGENT_INFO_MANAGEMENT_INTERNAL_H__

#include "du_agent_sdk/agent_module_interface.h" // ADUC_AGENT_MODULE_HANDLE, ADUC_AGENT_MODULE_INTERFACE
#include "aduc/c_utils.h" // EXTERN_C_* macros
#include <mosquitto.h> // mosquitto related functions
#include <mqtt_protocol.h> // mosquitto_property
#include "aducpal/time.h" // time_t
#include "parson.h" // JSON_Value, JSON_Object

EXTERN_C_BEGIN


typedef enum tagADUC_AGENT_INFO_WORKFLOW_STATE
{
    ADUC_AGENT_INFO_WORKFLOW_STATE_UNKNOWN = 0,
    ADUC_AGENT_INFO_WORKFLOW_STATE_INITIALIZED = 1,
    ADUC_AGENT_INFO_WORKFLOW_STATE_SUBSCRIBING = 2,
    ADUC_AGENT_INFO_WORKFLOW_STATE_SUBSCRIBED = 3,
} ADUC_AGENT_INFO_WORKFLOW_STATE;

typedef struct tagADUC_AgentToServiceMessage
{
    char* correlationId;
    char* messageType;
    char* contentType;
    char* content;
} ADUC_AgentToServiceMessage;

typedef struct tagADUC_AGENT_INFO_MANAGEMENT_STATE
{
    ADUC_AGENT_INFO_WORKFLOW_STATE workflowState;

    JSON_Value* agentInfoValue;
    char* subscribeTopic;
    char* publishTopic;
    char* ainfo_req_CorelationId;
    time_t ainfo_req_AttemptTime;
    time_t ainfo_req_NextAttemptTime;
    time_t ainfo_req_LastSuccessTime;
    time_t ainfo_req_LastErrorTime;

    time_t ainfo_resp_ReceivedTime;
    char* ainfo_resp_Content;
} ADUC_AGENT_INFO_MANAGEMENT_STATE;

DECLARE_AGENT_MODULE_PRIVATE(ADUC_Agent_Info_Management)


EXTERN_C_END

#endif // __ADUC_AGENT_INFO_MANAGEMENT_INTERNAL_H__
