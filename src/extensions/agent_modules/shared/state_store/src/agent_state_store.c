/**
 * @file agent_state_store.c
 * @brief Implements the Device Update agent state store functionality.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/agent_state_store.h"
#include <aduc/logging.h>
#include <aduc/string_c_utils.h> // IsNullOrEmpty
#include <limits.h> // HOST_NAME_MAX
#include <semaphore.h>
#include <stdbool.h> // bool
#include <stdlib.h>
#include <string.h>

#define ADUC_DEFAULT_DEVICE_REGISTRATION_STATE_POLL_INTERVAL_SECONDS (10)

// Internal state data
static bool isInitialized = false;

// MQTT SPEC says topics can have max len of 65536 utf-8 encoded bytes.
// Using the max mqtt topic byte length (can be lower in characters
// since utf-8 char can be encoded with up to 4 bytes)
#define MAX_ADU_MQTT_TOPIC_BYTE_LEN 65536

// The maximum [external] device ID provided by DPS is currently 1016 utf-8 characters.
// The charset is currently not restricted so allowing for any unicode codepoint where
// the utf-8 encoding could be up to 4 bytes for a single character.
#define MAX_ADU_DEVICE_ID_BYTE_LEN 1016 * 4

// scopeId, or adu account name, is 24 utf-8 chars. Multiplying by max utf-8 byte
// encoding size (4) to avoid precluding future scenarios.
// https://learn.microsoft.com/en-us/azure/iot-hub-device-update/device-update-limits
#define MAX_ADU_SCOPE_ID_BYTE_LEN 24 * 4

// TODO make this configurable.
//#define DEFAULT_STATE_STORE_PATH "/var/lib/adu/agent-state.json"
#define DEFAULT_STATE_STORE_PATH "/tmp/adu/agent-state.json"

#define STATE_FIELD_NAME_DEVICE_ID "deviceId"
#define STATE_FIELD_NAME_EXTERNAL_DEVICE_ID "externalDeviceId"
#define STATE_FIELD_NAME_MQTT_BROKER_HOSTNAME "mqttBrokerHostname"
#define STATE_FIELD_NAME_IS_DEVICE_ENROLLED "isDeviceEnrolled"
#define STATE_FIELD_NAME_IS_AGENT_INFO_REPORTED "isAgentInfoReported"
#define STATE_FIELD_NAME_IS_DEVICE_REGISTERED "isDeviceRegistered"
#define STATE_FIELD_NAME_DU_SERVICE_INSTANCE "duServiceInstance"

// Semaphore for process synchronization
static sem_t state_semaphore;

typedef struct
{
    void* communicationChannelHandle;
    void* updateWorkqueueHandle;
    void* reportingWorkqueueHandle;
    char* stateFilePath;
    char* deviceId;
    char* externalDeviceId;
    char* mqttBrokerHostname;
    char* scopeId;
    char* nonscopedTopic;
    char* scopedTopic;
    bool isDeviceEnrolled;
    bool isAgentInfoReported;
    bool isDeviceProvisionedByService;
    bool isUpdateResultsAck;
} ADUC_StateData;

static ADUC_StateData state = { 0 }; // Initialize to zero.

static char* SafeStrdup(const char* str)
{
    if (IsNullOrEmpty(str))
    {
        return NULL;
    }
    return strdup(str);
}

/**
 * @brief Initialize the state store.
 * @param state_file_path The path to the state file.
 * @param usingProvisioningService true if using service to provision device; otherwise, using config-based provisioning.
 * @return ADUC_STATE_STORE_RESULT_OK on success, ADUC_STATE_STORE_RESULT_ERROR on failure.
 * @remark The assumption is that only a single thread is every calling this.
 */
ADUC_STATE_STORE_RESULT ADUC_StateStore_Initialize(const char* state_file_path, bool usingProvisioningService)
{
    ADUC_STATE_STORE_RESULT res = ADUC_STATE_STORE_RESULT_ERROR;

    if (isInitialized)
    {
        Log_Info("State store already initialized.");
        return ADUC_STATE_STORE_RESULT_OK;
    }

    memset(&state, 0, sizeof(state));
    state.isDeviceProvisionedByService = usingProvisioningService;

    if (sem_init(&state_semaphore, 0, 1) != 0)
    {
        Log_Error("Failed to initialize semaphore.");
        res = ADUC_STATE_STORE_RESULT_ERROR;
        goto done;
    }

    Log_Info("State store initialized successfully.");

    isInitialized = true;
    res = ADUC_STATE_STORE_RESULT_OK;

done:

    return res;
}

/**
 * @brief Deinitialize the state store.
 * @remark The pre-requisite to calling this is to ensure all other threads using the store have been terminated.
 */
void ADUC_StateStore_Deinitialize()
{
    if (isInitialized)
    {
        free(state.deviceId);
        free(state.externalDeviceId);
        free(state.mqttBrokerHostname);
        free(state.scopeId);

        Log_Info("State store terminated successfully.");
        isInitialized = false;

        sem_destroy(&state_semaphore);
    }
}

char* ADUC_StateStore_GetExternalDeviceId()
{
    char* copy = NULL;
    sem_wait(&state_semaphore);

    const size_t externalDeviceIdByteLen = ADUC_StrNLen(state.externalDeviceId, MAX_ADU_DEVICE_ID_BYTE_LEN);
    if (externalDeviceIdByteLen != 0 && externalDeviceIdByteLen != MAX_ADU_DEVICE_ID_BYTE_LEN)
    {
        MallocAndSubstr(&copy, state.externalDeviceId, externalDeviceIdByteLen);
    }

    sem_post(&state_semaphore);
    return copy;
}

ADUC_STATE_STORE_RESULT ADUC_StateStore_SetExternalDeviceId(const char* externalDeviceId)
{
    sem_wait(&state_semaphore);

    free(state.externalDeviceId);
    state.externalDeviceId = SafeStrdup(externalDeviceId);

    sem_post(&state_semaphore);
    return ADUC_STATE_STORE_RESULT_OK;
}

/**
 * @brief Get the 'IsDeviceRegistered' value in the state store.
 * @return The value of 'IsDeviceRegistered' or false if not found.
 */
bool ADUC_StateStore_GetIsDeviceRegistered()
{
    bool value;

    sem_wait(&state_semaphore);

    value = !IsNullOrEmpty(state.externalDeviceId) && !IsNullOrEmpty(state.mqttBrokerHostname);

    sem_post(&state_semaphore);
    return value;
}

/**
 * @brief Get the recommended polling interval for device registration state.
 * @return The recommended polling interval in seconds.
 */
int ADUC_StateStore_GetDeviceRegistrationStatePollIntervalSeconds()
{
    return ADUC_DEFAULT_DEVICE_REGISTRATION_STATE_POLL_INTERVAL_SECONDS;
}

/**
 * @brief Get the scope identifier.
 * @return NULL if never set; else, the current scope id.
 */
char* ADUC_StateStore_GetScopeId()
{
    char* copy = NULL;
    sem_wait(&state_semaphore);

    const size_t scopeIdByteLen = ADUC_StrNLen(state.scopeId, MAX_ADU_SCOPE_ID_BYTE_LEN);
    if (scopeIdByteLen != 0 && scopeIdByteLen != MAX_ADU_SCOPE_ID_BYTE_LEN)
    {
        MallocAndSubstr(&copy, state.scopeId, scopeIdByteLen);
    }

    sem_post(&state_semaphore);
    return copy;
}

/**
 * @brief Set the scope Id.
 * @param scopeId The scope id.
 * @return ADUC_STATE_STORE_RESULT_OK on success, ADUC_STATE_STORE_RESULT_ERROR on failure.
 */
ADUC_STATE_STORE_RESULT ADUC_StateStore_SetScopeId(const char* scopeId)
{
    ADUC_STATE_STORE_RESULT result = ADUC_STATE_STORE_RESULT_ERROR;

    const size_t newScopeIdByteLen = ADUC_StrNLen(scopeId, MAX_ADU_SCOPE_ID_BYTE_LEN);
    if (newScopeIdByteLen == 0 || newScopeIdByteLen == MAX_ADU_SCOPE_ID_BYTE_LEN)
    {
        return result;
    }

    sem_wait(&state_semaphore);

    char* prevScopeId = state.scopeId;

    if (MallocAndSubstr(&state.scopeId, scopeId, newScopeIdByteLen))
    {
        free(prevScopeId);
        result = ADUC_STATE_STORE_RESULT_OK;
    }
    else
    {
        state.scopeId = prevScopeId;
    }

    sem_post(&state_semaphore);
    return result;
}

/**
 * @brief Whether the scoped or the non-scoped topic is currently subscribed to.
 *
 * @param topic The topic in question.
 * @param isScoped Whether this is the single scoped topic, or the single non-scoped topic.
 * @return ADUC_STATE_STORE_TOPIC_SUBSCRIBE_STATUS The topic subscribe status.
 */
bool ADUC_StateStore_GetTopicSubscribedStatus(const char* topic, bool isScoped)
{
    bool result = false;
    sem_wait(&state_semaphore);

    if (isScoped)
    {
        if (state.scopedTopic != NULL && strncmp(state.scopedTopic, topic, MAX_ADU_MQTT_TOPIC_BYTE_LEN) == 0)
        {
            result = true;
        }
    }
    else
    {
        if (state.nonscopedTopic != NULL && strncmp(state.nonscopedTopic, topic, MAX_ADU_MQTT_TOPIC_BYTE_LEN) == 0)
        {
            result = true;
        }
    }

    sem_post(&state_semaphore);
    return result;
}

/**
 * @brief Sets the subscribe status of the scoped or the non-scoped topic.
 *
 * @param topic The topic.
 * @param isScoped True to set the subscribed status of the single scoped topic. False to set the subscribed status of the single non-scoped topic.
 * @param newStatus The new subscribe status for the topic.
 * @return ADUC_STATE_STORE_RESULT_OK on successful setting of subscribe/unsubscribe status.
 * @return ADUC_STATE_STORE_RESULT_ERROR_EMPTY_TOPIC if the topic is an empty or null string.
 * @return ADUC_STATE_STORE_RESULT_ERROR_MAX_TOPIC_BYTE_LENGTH_EXCEEDED if the utf-8 topic string's byte length exceeds MAX_ADU_MQTT_TOPIC_BYTE_LEN
 * @return ADUC_STATE_STORE_RESULT_UNKNOWN_TOPIC when trying to unsubscribe from the scoped or non-scoped topic that is not currently subscribed.
 * @return ADUC_STATE_STORE_RESULT_ERROR when failed subscribe/unsubscribe an existing topic for some other reason.
 */
ADUC_STATE_STORE_RESULT ADUC_StateStore_SetTopicSubscribedStatus(const char* topic, bool isScoped, bool subscribed)
{
    ADUC_STATE_STORE_RESULT result = ADUC_STATE_STORE_RESULT_UNKNOWN_TOPIC;
    size_t topic_len = 0;

    sem_wait(&state_semaphore);

    topic_len = ADUC_StrNLen(topic, MAX_ADU_MQTT_TOPIC_BYTE_LEN);

    if (topic_len == 0)
    {
        result = ADUC_STATE_STORE_RESULT_ERROR_EMPTY_TOPIC;
        goto done;
    }

    if (topic_len == MAX_ADU_MQTT_TOPIC_BYTE_LEN)
    {
        result = ADUC_STATE_STORE_RESULT_ERROR_MAX_TOPIC_BYTE_LENGTH_EXCEEDED;
        goto done;
    }

    char** state_topic_target = isScoped ? &state.scopedTopic : &state.nonscopedTopic;

    // Need to free and null-out regardless of subscribed value.
    free(*state_topic_target);
    *state_topic_target = NULL;

    if (subscribed)
    {
        *state_topic_target = (char*)calloc(topic_len + 1, sizeof(char));
        if (!ADUC_Safe_StrCopyN(*state_topic_target, topic, topic_len + 1, topic_len))
        {
            result = ADUC_STATE_STORE_RESULT_ERROR_MAX_TOPIC_BYTE_LENGTH_EXCEEDED;
            goto done;
        }
    }

    result = ADUC_STATE_STORE_RESULT_OK;
done:

    sem_post(&state_semaphore);
    return result;
}

bool ADUC_StateStore_IsDeviceEnrolled()
{
    sem_wait(&state_semaphore);
    bool value = state.isDeviceEnrolled;
    sem_post(&state_semaphore);
    return value;
}

ADUC_STATE_STORE_RESULT ADUC_StateStore_SetIsDeviceEnrolled(bool isDeviceEnrolled)
{
    sem_wait(&state_semaphore);
    state.isDeviceEnrolled = isDeviceEnrolled;
    sem_post(&state_semaphore);
    return ADUC_STATE_STORE_RESULT_OK;
}

bool ADUC_StateStore_IsAgentInfoReported()
{
    bool reported = false;

    sem_wait(&state_semaphore);
    reported = state.isAgentInfoReported;
    sem_post(&state_semaphore);
    return reported;
}

ADUC_STATE_STORE_RESULT ADUC_StateStore_SetIsAgentInfoReported(bool isAgentInfoReported)
{
    sem_wait(&state_semaphore);
    state.isAgentInfoReported = isAgentInfoReported;
    sem_post(&state_semaphore);
    return ADUC_STATE_STORE_RESULT_OK;
}

/**
 * @brief Get the communication channel handle.
 * @return void* The communcation handle.
 */
const void* ADUC_StateStore_GetCommunicationChannelHandle()
{
    void* h = NULL;

    sem_wait(&state_semaphore);

    h = state.communicationChannelHandle;

    sem_post(&state_semaphore);
    return h;
}

/**
 * @brief Set the communication channel handle. Clearing with NULL is allowed between sessions.
 * @param commChannelHandle The communication channel handle to set.
 * @return ADUC_STATE_STORE_RESULT The store result.
 * @remark More than one communication channel is NOT currently supported.
 */
ADUC_STATE_STORE_RESULT ADUC_StateStore_SetCommunicationChannelHandle(void* commChannelHandle)
{
    sem_wait(&state_semaphore);

    state.communicationChannelHandle = commChannelHandle;

    sem_post(&state_semaphore);
    return ADUC_STATE_STORE_RESULT_OK;
}

/**
 * @brief Get the MQTT broker hostname.
 * @returns
 */
char* ADUC_StateStore_GetMQTTBrokerHostname()
{
    char* copy = NULL;
    sem_wait(&state_semaphore);

    const size_t hostnameByteLen = ADUC_StrNLen(state.mqttBrokerHostname, HOST_NAME_MAX);
    if (hostnameByteLen != 0 && hostnameByteLen != HOST_NAME_MAX)
    {
        MallocAndSubstr(&copy, state.mqttBrokerHostname, hostnameByteLen);
    }

    sem_post(&state_semaphore);
    return copy;
}

/**
 * @brief Set the MQTT broker hostname. Can be reset by passing in NULL.
 * @param hostname The hostname.
 * @return ADUC_STATE_STORE_RESULT_OK on success, ADUC_STATE_STORE_RESULT_ERROR on failure.
 */
ADUC_STATE_STORE_RESULT ADUC_StateStore_SetMQTTBrokerHostname(const char* hostname)
{
    ADUC_STATE_STORE_RESULT result = ADUC_STATE_STORE_RESULT_ERROR;
    sem_wait(&state_semaphore);

    const size_t hostnameByteLen = ADUC_StrNLen(hostname, HOST_NAME_MAX);
    if (hostnameByteLen != 0 && hostnameByteLen != HOST_NAME_MAX)
    {
        char* prevHostname = state.mqttBrokerHostname;

        if (MallocAndSubstr(&state.mqttBrokerHostname, hostname, hostnameByteLen))
        {
            free(prevHostname);
            prevHostname = NULL;
            result = ADUC_STATE_STORE_RESULT_OK;
        }
        else
        {
            state.mqttBrokerHostname = prevHostname;
            prevHostname = NULL;
        }
    }

    sem_post(&state_semaphore);
    return result;
}

void* ADUC_StateStore_GetUpdateWorkQueueHandle()
{
    return state.updateWorkqueueHandle;
}

void ADUC_StateStore_SetUpdateWorkQueueHandle(void* handle)
{
    state.updateWorkqueueHandle = handle;
}

void* ADUC_StateStore_GetReportingWorkQueueHandle()
{
    return state.reportingWorkqueueHandle;
}

void ADUC_StateStore_SetReportingWorkQueueHandle(void* handle)
{
    state.reportingWorkqueueHandle = handle;
}

bool ADUC_StateStore_IsReportResultsAck()
{
    return state.isUpdateResultsAck;
}

void ADUC_StateStore_SetReportResultsAck(bool isAck)
{
    state.isUpdateResultsAck = isAck;
}
