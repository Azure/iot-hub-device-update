#ifndef ADU_MQTT_COMMON_H
#define ADU_MQTT_COMMON_H

#include <aduc/adu_module_state.h> // ADUC_MQTT_CLIENT_MODULE_STATE
#include <aduc/c_utils.h> // EXTERN_C_*
#include "aduc/mqtt_broker_common.h" // ADUC_MQTT_Message_Context
#include <aduc/retry_utils.h> // ADUC_Retriable_Operation_Context
#include <du_agent_sdk/agent_module_interface.h> // ADUC_AGENT_MODULE_HANDLE

EXTERN_C_BEGIN

ADUC_Retriable_Operation_Context* OperationContextFromAgentModuleHandle(ADUC_AGENT_MODULE_HANDLE handle);
bool MqttTopicSetupNeeded(ADUC_Retriable_Operation_Context* context, ADUC_MQTT_Message_Context* messageContext, bool isScoped);
bool CommunicationChannelNeededSetup(ADUC_Retriable_Operation_Context* context);
bool ExternalDeviceIdSetupNeeded(ADUC_Retriable_Operation_Context* context);
bool SettingUpAduMqttRequestPrerequisites(ADUC_Retriable_Operation_Context* context, ADUC_MQTT_Message_Context* messageContext, bool isScoped);
bool ADUC_MQTT_COMMON_Ensure_Subscribed_for_Response(const ADUC_Retriable_Operation_Context* context, ADUC_MQTT_Message_Context* messageContext);

EXTERN_C_END

#endif // ADU_MQTT_COMMON_H
