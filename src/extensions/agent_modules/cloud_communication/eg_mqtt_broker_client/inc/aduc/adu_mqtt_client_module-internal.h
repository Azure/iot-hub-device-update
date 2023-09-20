/**
 * @file adu_mqtt_client_module_internal.h
 * @brief A header file for internal APIs for Device Update MQTT client module.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef __ADU_MQTT_CLIENT_MODULE_INTERNAL_H__
#    define __ADU_MQTT_CLIENT_MODULE_INTERNAL__

#    include "aduc/agent_module_interface.h" // ADUC_AGENT_MODULE_HANDLE

bool ADUC_MQTT_CLIENT_MODULE_IsDeviceEnrolled(ADUC_AGENT_MODULE_HANDLE handle);

bool ADUC_MQTT_CLIENT_MODULE_SetMQTTBrokerEndpoint(ADUC_AGENT_MODULE_HANDLE handle, const char* mqttBrokerEndpoint);

const char* ADUC_MQTT_CLIENT_MODULE_GetPublishTopic(ADUC_AGENT_MODULE_HANDLE handle);

#endif /* __ADU_MQTT_CLIENT_MODULE_H__ */
