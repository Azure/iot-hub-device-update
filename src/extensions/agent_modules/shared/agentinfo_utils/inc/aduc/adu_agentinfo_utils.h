#ifndef ADU_agentinfo_UTILS_H
#define ADU_agentinfo_UTILS_H

#include <aduc/adu_agentinfo.h> // ADUC_AgentInfo_Request_Operation_Data
#include <aduc/mqtt_broker_common.h> // ADU_MQTT_Message_Context
#include <aduc/c_utils.h> // EXTERN_C_*
#include <stdbool.h> // bool

EXTERN_C_BEGIN

ADUC_AgentInfo_Request_Operation_Data* AgentInfoData_FromOperationContext(ADUC_Retriable_Operation_Context* context);
void AgentInfoData_SetCorrelationId(ADUC_AgentInfo_Request_Operation_Data* agentInfoData, const char* correlationId);
bool Handle_AgentInfo_Response(ADUC_AgentInfo_Request_Operation_Data* agentInfoData, ADUC_Retriable_Operation_Context* context);

EXTERN_C_END

#endif // ADU_agentinfo_UTILS_H
