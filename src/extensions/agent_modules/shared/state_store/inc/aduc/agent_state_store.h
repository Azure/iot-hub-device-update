/**
 * @file agent_state_store.h
 * @brief Header file for Device Update agent state store functions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_MQTT_STATE_STORE_H
#define ADUC_MQTT_STATE_STORE_H

#include "aduc/c_utils.h"
#include "parson.h"
#include "stdbool.h"

EXTERN_C_BEGIN

#define ADUC_DEFAULT_DEVICE_REGISTRATION_STATE_POLL_INTERVAL_SECONDS (10)

/*
 * @brief Result codes for state store functions.
 */
typedef enum tagADUC_STATE_STORE_RESULT
{
    ADUC_STATE_STORE_RESULT_ERROR_MAX_TOPIC_BYTE_LENGTH_EXCEEDED = -4,
    ADUC_STATE_STORE_RESULT_ERROR_EMPTY_TOPIC = -3,
    ADUC_STATE_STORE_RESULT_UNKNOWN_TOPIC = -2,
    ADUC_STATE_STORE_RESULT_ERROR = -1,
    ADUC_STATE_STORE_RESULT_OK = 0,
} ADUC_STATE_STORE_RESULT;

typedef enum tagADUC_STATE_STORE_TOPIC_SUBSCRIBE_STATUS
{
    ADUC_STATE_STORE_TOPIC_SUBSCRIBE_STATUS_UNKNOWN = 0,
    ADUC_STATE_STORE_TOPIC_SUBSCRIBE_STATUS_SUBSCRIBED = 1,
    ADUC_STATE_STORE_TOPIC_SUBSCRIBE_STATUS_NOTSUBSCRIBED = 2,
} ADUC_STATE_STORE_TOPIC_SUBSCRIBE_STATUS;

ADUC_STATE_STORE_RESULT ADUC_StateStore_Initialize(const char* state_file_path, bool isUsingProvisioningService);
void ADUC_StateStore_Deinitialize();

const char* ADUC_StateStore_GetExternalDeviceId();
ADUC_STATE_STORE_RESULT ADUC_StateStore_SetExternalDeviceId(const char* externalDeviceId);

ADUC_STATE_STORE_RESULT ADUC_StateStore_SetIsDeviceRegistered(bool isDeviceRegistered);
bool ADUC_StateStore_GetIsDeviceRegistered();

int ADUC_StateStore_GetDeviceRegistrationStatePollIntervalSeconds();

const char* ADUC_StateStore_GetMQTTBrokerHostname();
ADUC_STATE_STORE_RESULT ADUC_StateStore_SetMQTTBrokerHostname(const char* hostname);

const char* ADUC_StateStore_GetDeviceId();
ADUC_STATE_STORE_RESULT ADUC_StateStore_SetDeviceId(const char* deviceId);

const char* ADUC_StateStore_GetScopeId();
ADUC_STATE_STORE_RESULT ADUC_StateStore_SetScopeId(const char* scopeId);

bool ADUC_StateStore_GetTopicSubscribedStatus(const char* topic, bool isScoped);
ADUC_STATE_STORE_RESULT ADUC_StateStore_SetTopicSubscribed(const char* topic, bool isScoped, bool subscribed);

bool ADUC_StateStore_IsDeviceEnrolled();
ADUC_STATE_STORE_RESULT ADUC_StateStore_SetIsDeviceEnrolled(bool isDeviceEnrolled);

bool ADUC_StateStore_IsAgentInfoReported();
ADUC_STATE_STORE_RESULT ADUC_StateStore_SetIsAgentInfoReported(bool isAgentInfoReported);

const void* ADUC_StateStore_GetCommunicationChannelHandle();
ADUC_STATE_STORE_RESULT ADUC_StateStore_SetCommunicationChannelHandle(void* communicationChannelHandler);

EXTERN_C_END

#endif // ADUC_MQTT_STATE_STORE_H
