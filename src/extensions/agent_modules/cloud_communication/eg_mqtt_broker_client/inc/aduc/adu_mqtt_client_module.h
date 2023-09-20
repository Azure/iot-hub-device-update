/**
 * @file adu_mqtt_client_module.h
 * @brief A header file for the Device Update MQTT client module.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef __ADU_MQTT_CLIENT_MODULE_H__
#define __ADU_MQTT_CLIENT_MODULE_H__

#include "aduc/adu_mqtt_protocol.h" // for ADUC_COMMUNICATION_*, ADUC_ENROLLMENT_*, ADUC_MQTT_MESSAGE_*, ADUC_MQTT_SETTINGS, ADUC_Result
#include "aduc/mqtt_client.h" // for ADUC_MQTT_SETTINGS
#include "aduc/result.h" // for ADUC_Result
#include "aducpal/time.h" // for time_t
#include "du_agent_sdk/agent_module_interface.h" // for ADUC_AGENT_MODULE_HANDLE
#include <mosquitto.h> // for mosquitto related functions
#include <mqtt_protocol.h> // for mosquitto_property

/**
 * @brief The module state.
 */
typedef struct ADUC_MQTT_CLIENT_MODULE_STATE_TAG
{
    void* moduleInitData; //!< Module init data
    ADUC_MQTT_SETTINGS mqttSettings; //!< MQTT connection settings

    char* mqtt_topic_service2agent; // For v1, we're only subscribe to 1 topic.
    char* mqtt_topic_agent2service; // For v1, we're only publish to 1 topic.

    ADU_MQTT_CLIENT_MODULE_INITIALIZE_STATE initializeState; //!< Module initialize status.
    ADU_ENROLLMENT_STATE enrollmentState; //!< Enrollment state

    ADU_MQTT_MESSAGE_INFO enrollmentMessageInfo; //!< Enrollment message info

    time_t lastErrorTime; //!< Last time an error occurred
    ADUC_Result lastAducResult; //!< Last ADUC result
    int lastError; //!< Last error code
    time_t nextOperationTime; //!< Next time to perform an operation

} ADUC_MQTT_CLIENT_MODULE_STATE;

/**
 * @brief Sets the MQTT broker endpoint.
 */
int ADUC_MQTT_CLIENT_MODULE_SetMQTTBrokerEndpoint(ADUC_AGENT_MODULE_HANDLE* handle, const char* mqttBrokerEndpoint);

/**
 * @brief Free resources allocated for the module state.
 * @param state The module state to free.
 */
void ADUC_MQTT_CLIENT_MODULE_STATE_Free(ADUC_MQTT_CLIENT_MODULE_STATE* state);

#endif /* __ADU_MQTT_CLIENT_MODULE_H__ */
