
#ifndef AGENTINFO_REQUEST_OPERATION_H
#define AGENTINFO_REQUEST_OPERATION_H

#include <aduc/retry_utils.h> // ADUC_Retriable_Operation_Context, ADUC_Retry_Params
#include <aduc/mqtt_broker_common.h> // ADUC_MQTT_Message_Context
#include <aducpal/sys_time.h> // time_t
#include <du_agent_sdk/agent_module_interface.h> // ADUC_AGENT_MODULE_HANDLE

ADUC_Retriable_Operation_Context* CreateAndInitializeAgentInfoRequestOperation();

#endif // AGENTINFO_REQUEST_OPERATION_H
