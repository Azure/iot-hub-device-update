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
#include "aduc/result.h"      // for ADUC_Result
#include "aducpal/time.h"     // for time_t
#include <mosquitto.h>        // for mosquitto related functions
#include <mqtt_protocol.h>    // for mosquitto_property

/**
 * @brief The module state.
 */
typedef struct ADUC_MQTT_CLIENT_MODULE_STATE_TAG
{
    struct mosquitto* mqttClient; //!< Mosquitto client
    bool initialized; //!< Module is initialized

    time_t commChannelStateChangeTime; //!< Time of last communication channel state change
    time_t commChannelLastAttemptTime; //!< Last time an attempt was made to connect to the MQTT broker
    time_t commChannelLastConnectedTime; //!< Last time connected to MQTT broker

    bool subscribed; //!< Device is subscribed to DPS topics

    ADU_ENROLLMENT_STATE enrollmentState; //!< Enrollment state
    ADU_MQTT_MESSAGE_INFO enrollmentMessageInfo; //!< Enrollment message info

    //char* regRequestId; //!< Registration request ID
    time_t lastErrorTime; //!< Last time an error occurred
    ADUC_Result lastAducResult; //!< Last ADUC result
    int lastError; //!< Last error code
    ADUC_MQTT_SETTINGS mqttSettings; //!< DPS settings
    time_t nextOperationTime; //!< Next time to perform an operation

} ADUC_MQTT_CLIENT_MODULE_STATE;


#endif /* __ADU_MQTT_CLIENT_MODULE_H__ */
