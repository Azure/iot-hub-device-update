#ifndef ADU_agentinfo_UTILS_H
#define ADU_agentinfo_UTILS_H

#include <aduc/retry_utils.h> // ADUC_Retriable_Operation_Context
#include <parson.h> // JSON_Value

EXTERN_C_BEGIN

bool Handle_AgentInfo_Response(ADUC_MQTT_Message_Context* context, int resultCode);

EXTERN_C_END

#endif // ADU_agentinfo_UTILS_H
