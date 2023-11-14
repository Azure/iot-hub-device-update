#ifndef ADU_MQTT_COMMON_H
#define ADU_MQTT_COMMON_H

#include "aduc/mqtt_broker_common.h" // ADUC_MQTT_Message_Context
#include <aduc/c_utils.h> // EXTERN_C_*
#include <aduc/retry_utils.h> // ADUC_Retriable_Operation_Context

EXTERN_C_BEGIN

ADUC_Retriable_Operation_Context* operationContextFromAgentModuleHandle(ADUC_AGENT_MODULE_HANDLE handle);
bool SettingUpAduMqttRequestPrerequisites(ADUC_Retriable_Operation_Context* context, ADUC_MQTT_Message_Context* messageContext);
bool ADUC_MQTT_COMMON_Ensure_Subscribed_for_Response(const ADUC_Retriable_Operation_Context* context, ADUC_MQTT_Message_Context* messageContext);

EXTERN_C_END

#endif // ADU_MQTT_COMMON_H
