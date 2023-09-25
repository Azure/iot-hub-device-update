/**
 * @file agent_state_store.c
 * @brief Implements the Device Update agent state store functionality.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/agent_state_store.h"
#include "aduc/logging.h"
#include "aduc/string_c_utils.h" // IsNuLLOrEmpty
#include "parson.h"
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>

#include "stdbool.h" // bool

// Internal state data
static JSON_Value* inmem_state_data = NULL;

static JSON_Value* durable_state_data = NULL;

static bool isInitialized = false;

// TODO make this configurable.
#define DEFAULT_STATE_STORE_PATH "/var/lib/adu/agent-state.json"

// Semaphore for process synchronization
static sem_t state_semaphore;

typedef struct
{
    char* externalDeviceId;
    char* deviceId;
    bool isDeviceEnrolled;
    bool isAgentInfoReported;
} ADUC_StateData;

static ADUC_StateData state = { 0 }; // Initialize to zero.

static char* SafeStrdup(const char* str)
{
    if (str == NULL)
    {
        return NULL;
    }
    return strdup(str);
}

/**
 * @brief Initialize the state store.
 * @param state_file_path The path to the state file.
 * @return ADUC_STATE_STORE_RESULT_OK on success, ADUC_STATE_STORE_RESULT_ERROR on failure.
 */
ADUC_STATE_STORE_RESULT ADUC_StateStore_Initialize(const char* state_file_path)
{
    if (isInitialized)
    {
        Log_Info("State store already initialized.");
        return ADUC_STATE_STORE_RESULT_OK;
    }

    if (sem_init(&state_semaphore, 0, 1) != 0)
    {
        Log_Error("Failed to initialize semaphore.");
        json_value_free(inmem_state_data);
        inmem_state_data = NULL;
        return ADUC_STATE_STORE_RESULT_ERROR;
    }

    ADUC_STATE_STORE_RESULT res = ADUC_STATE_STORE_RESULT_ERROR;
    sem_wait(&state_semaphore);

    inmem_state_data = json_value_init_object();
    if (!inmem_state_data)
    {
        Log_Error("Failed to initialize state data store.");
        goto done;
    }

    durable_state_data = json_value_init_object();
    if (!durable_state_data)
    {
        Log_Error("Failed to initialize persisted state data store.");
        goto done;
    }

    // Loading from the file.
    JSON_Value* durable_state_data = json_parse_file(state_file_path);

    if (durable_state_data == NULL)
    {
        Log_Warn("Failed to load state data from file %s. Creating...", state_file_path);
    }

    if (durable_state_data == NULL)
    {
        durable_state_data = json_value_init_object();
    }

    if (!durable_state_data)
    {
        Log_Error("Failed to initialize state data store.");
        goto done;
    }

    JSON_Object* state_data_obj = json_value_get_object(durable_state_data);
    state.externalDeviceId = SafeStrdup(json_object_get_string(state_data_obj, "externalDeviceId"));
    state.deviceId = SafeStrdup(json_object_get_string(state_data_obj, "deviceId"));
    state.isDeviceEnrolled = json_object_get_boolean(state_data_obj, "isDeviceEnrolled");
    state.isAgentInfoReported = json_object_get_boolean(state_data_obj, "isAgentInfoReported");

    Log_Info("State store initialized successfully.");
    isInitialized = true;
    res = ADUC_STATE_STORE_RESULT_OK;

done:
    if (!isInitialized)
    {
        json_value_free(inmem_state_data);
        inmem_state_data = NULL;
        json_value_free(durable_state_data);
        durable_state_data = NULL;
    }
    sem_post(&state_semaphore);
    return res;
}

static JSON_Object* GetRootObject(bool durable)
{
    return json_value_get_object(durable ? durable_state_data : inmem_state_data);
}

JSON_Value* ADUC_StateStore_GetData(bool durable, const char* key)
{
    sem_wait(&state_semaphore);
    JSON_Value* result = json_object_dotget_value(GetRootObject(durable), key);
    sem_post(&state_semaphore);
    return result;
}

ADUC_STATE_STORE_RESULT ADUC_StateStore_SetData(bool durable, const char* key, JSON_Value* value)
{
    if (!key || !value)
    {
        Log_Error("Invalid input parameters for SetData.");
        return -1;
    }

    sem_wait(&state_semaphore);
    if (json_object_dotset_value(GetRootObject(durable), key, value) != JSONSuccess)
    {
        Log_Error("Failed to set data for key %s.", key);
        sem_post(&state_semaphore);
        return -1;
    }
    sem_post(&state_semaphore);
    Log_Info("Data set successfully for key %s.", key);
    return 0;
}

ADUC_STATE_STORE_RESULT ADUC_StateStore_WriteToFile(const char* filename)
{
    sem_wait(&state_semaphore);
    if (json_serialize_to_file(inmem_state_data, filename) != JSONSuccess)
    {
        Log_Error("Failed to write state data to file %s.", filename);
        sem_post(&state_semaphore);
        return ADUC_STATE_STORE_RESULT_ERROR;
    }
    sem_post(&state_semaphore);
    Log_Info("Successfully wrote state data to file %s.", filename);
    return ADUC_STATE_STORE_RESULT_OK;
}

void ADUC_StateStore_Deinitialize(const char* state_file_path)
{
    sem_wait(&state_semaphore);

    // Saving to the file
    JSON_Object* root_object = GetRootObject(true /* durable */);

    json_object_set_string(root_object, "externalDeviceId", state.externalDeviceId);
    json_object_set_string(root_object, "deviceId", state.deviceId);
    json_object_set_boolean(root_object, "isDeviceEnrolled", state.isDeviceEnrolled);
    json_object_set_boolean(root_object, "isAgentInfoReported", state.isAgentInfoReported);

    json_serialize_to_file(durable_state_data, state_file_path);

    json_value_free(durable_state_data);
    durable_state_data = NULL;
    json_value_free(inmem_state_data);
    inmem_state_data = NULL;

    free(state.externalDeviceId);
    free(state.deviceId);

    sem_post(&state_semaphore);
    sem_destroy(&state_semaphore);
    Log_Info("State store terminated successfully.");
    isInitialized = false;
}

// Integer data retrieval and setting
ADUC_STATE_STORE_RESULT ADUC_StateStore_GetInt(bool durable, const char* key, int* out_value)
{
    if (IsNullOrEmpty(key) || out_value == NULL)
    {
        Log_Error("Invalid input (key=%p, out_value=%p).", key, out_value);
        return ADUC_STATE_STORE_RESULT_ERROR;
    }

    sem_wait(&state_semaphore);
    JSON_Object* root_object = GetRootObject(durable);
    double result = json_object_dotget_number(root_object, key);

    if (json_object_dotget_value(root_object, key) == NULL
        || json_value_get_type(json_object_dotget_value(root_object, key)) != JSONNumber)
    {
        Log_Warn("Failed to retrieve int value for key %s.", key);
        sem_post(&state_semaphore);
        return ADUC_STATE_STORE_RESULT_ERROR;
    }

    *out_value = (int)result;
    sem_post(&state_semaphore);
    Log_Info("Successfully retrieved int value for key %s.", key);
    return ADUC_STATE_STORE_RESULT_OK;
}

ADUC_STATE_STORE_RESULT ADUC_StateStore_SetInt(bool durable, const char* key, int value)
{
    return ADUC_StateStore_SetData(durable, key, json_value_init_number(value));
}

// Unsigned integer data retrieval and setting
ADUC_STATE_STORE_RESULT ADUC_StateStore_GetUnsignedInt(bool durable, const char* key, unsigned int* out_value)
{
    int result;
    int status = ADUC_StateStore_GetInt(durable, key, &result);
    if (status == 0 && result >= 0)
    {
        *out_value = (unsigned int)result;
        Log_Info("Successfully retrieved unsigned int value for key %s.", key);
        return ADUC_STATE_STORE_RESULT_OK;
    }
    Log_Warn("Failed to retrieve unsigned int value for key %s.", key);
    return ADUC_STATE_STORE_RESULT_ERROR;
}

ADUC_STATE_STORE_RESULT ADUC_StateStore_SetUnsignedInt(bool durable, const char* key, unsigned int value)
{
    return ADUC_StateStore_SetInt(durable, key, (int)value); // Note: Assumes the value is within the range of 'int'
}

// String data retrieval and setting
ADUC_STATE_STORE_RESULT ADUC_StateStore_GetString(bool durable, const char* key, char** out_value)
{
    if (IsNullOrEmpty(key) || out_value == NULL)
    {
        Log_Error("Invalid input (key=%p, out_value=%p).", key, out_value);
        return ADUC_STATE_STORE_RESULT_ERROR;
    }

    sem_wait(&state_semaphore);
    JSON_Object* root_object = GetRootObject(durable);
    const char* result = json_object_dotget_string(root_object, key);

    if (!result)
    {
        Log_Warn("Failed to retrieve string value for key %s.", key);
        sem_post(&state_semaphore);
        return ADUC_STATE_STORE_RESULT_ERROR;
    }

    *out_value = SafeStrdup(result); // Callers are responsible for freeing this memory.
    sem_post(&state_semaphore);
    Log_Info("Successfully retrieved string value for key %s.", key);
    return ADUC_STATE_STORE_RESULT_OK;
}

ADUC_STATE_STORE_RESULT ADUC_StateStore_SetString(bool durable, const char* key, const char* value)
{
    return ADUC_StateStore_SetData(durable, key, json_value_init_string(value));
}

// Boolean data retrieval and setting
ADUC_STATE_STORE_RESULT ADUC_StateStore_GetBool(bool durable, const char* key, int* out_value)
{
    if (IsNullOrEmpty(key) || out_value == NULL)
    {
        Log_Error("Invalid input (key=%p, out_value=%p).", key, out_value);
        return ADUC_STATE_STORE_RESULT_ERROR;
    }

    sem_wait(&state_semaphore);
    JSON_Object* root_object = GetRootObject(durable);
    int result = json_object_dotget_boolean(root_object, key);

    if (json_object_dotget_value(root_object, key) == NULL
        || json_value_get_type(json_object_dotget_value(root_object, key)) != JSONBoolean)
    {
        Log_Warn("Failed to retrieve bool value for key %s.", key);
        sem_post(&state_semaphore);
        return ADUC_STATE_STORE_RESULT_ERROR;
    }

    *out_value = result;
    sem_post(&state_semaphore);
    Log_Info("Successfully retrieved bool value for key %s.", key);
    return ADUC_STATE_STORE_RESULT_OK;
}

ADUC_STATE_STORE_RESULT ADUC_StateStore_SetBool(bool durable, const char* key, int value)
{
    return ADUC_StateStore_SetData(durable, key, json_value_init_boolean(value));
}

const char* ADUC_StateStore_GetExternalDeviceId(void)
{
    sem_wait(&state_semaphore);
    const char* value = state.externalDeviceId;
    sem_post(&state_semaphore);
    return value;
}

ADUC_STATE_STORE_RESULT ADUC_StateStore_SetExternalDeviceId(const char* externalDeviceId)
{
    if (!externalDeviceId)
    {
        Log_Error("Invalid input for SetExternalDeviceId.");
        return ADUC_STATE_STORE_RESULT_ERROR;
    }

    sem_wait(&state_semaphore);
    free(state.externalDeviceId);
    state.externalDeviceId = SafeStrdup(externalDeviceId);
    sem_post(&state_semaphore);
    return ADUC_STATE_STORE_RESULT_OK;
}

const char* ADUC_StateStore_GetDeviceId(void)
{
    sem_wait(&state_semaphore);
    const char* value = state.deviceId;
    sem_post(&state_semaphore);
    return value;
}

ADUC_STATE_STORE_RESULT ADUC_StateStore_SetDeviceId(const char* deviceId)
{
    if (!deviceId)
    {
        Log_Error("Invalid input for SetDeviceId.");
        return ADUC_STATE_STORE_RESULT_ERROR;
    }

    sem_wait(&state_semaphore);
    free(state.deviceId);
    state.deviceId = SafeStrdup(deviceId);
    sem_post(&state_semaphore);
    return ADUC_STATE_STORE_RESULT_OK;
}

bool ADUC_StateStore_GetIsDeviceEnrolled(void)
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

bool ADUC_StateStore_GetIsAgentInfoReported(void)
{
    sem_wait(&state_semaphore);
    bool value = state.isAgentInfoReported;
    sem_post(&state_semaphore);
    return value;
}

ADUC_STATE_STORE_RESULT ADUC_StateStore_SetIsAgentInfoReported(bool isAgentInfoReported)
{
    sem_wait(&state_semaphore);
    state.isAgentInfoReported = isAgentInfoReported;
    sem_post(&state_semaphore);
    return ADUC_STATE_STORE_RESULT_OK;
}

/**
 * @brief Get a pointer to the communication channel handle.
 * @param sessionId The session ID  of the communication channel.
 * @return A pointer to the communication channel handle, or NULL if not found.
 */
const void* ADUC_StateStore_GetCommunicationChannelHandle(const char* sessionId)
{
    if (IsNullOrEmpty(sessionId))
    {
        Log_Error("Invalid input (sessionId=%p).", sessionId);
        return NULL;
    }

    sem_wait(&state_semaphore);
    JSON_Object* root_object = GetRootObject(false /* durable */);
    JSON_Object* commHandles =
        json_value_get_object(json_object_get_value(root_object, "communicationChannelHandles"));
    int* handle = (int*)(long)json_object_get_number(commHandles, sessionId);
    sem_post(&state_semaphore);
    return handle;
}

/**
 * @brief Set the communication channel handle.
 * @param sessionId The session ID of the communication channel.
 * @param communicationChannelHandler The communication channel handle to set.
 * @return ADUC_STATE_STORE_RESULT_OK on success, ADUC_STATE_STORE_RESULT_ERROR on failure.
 */
ADUC_STATE_STORE_RESULT
ADUC_StateStore_SetCommunicationChannelHandle(const char* sessionId, const void* communicationChannelHandler)
{
    if (IsNullOrEmpty(sessionId) || communicationChannelHandler == NULL)
    {
        Log_Error(
            "Invalid input (sessionId=%p, communicationChannelHandler=%p).", sessionId, communicationChannelHandler);
        return ADUC_STATE_STORE_RESULT_ERROR;
    }
    ADUC_STATE_STORE_RESULT res = ADUC_STATE_STORE_RESULT_ERROR;

    sem_wait(&state_semaphore);
    JSON_Object* root_object = GetRootObject(false /* durable */);
    JSON_Object* commHandles =
        json_value_get_object(json_object_get_value(root_object, "communicationChannelHandles"));
    if (commHandles == NULL)
    {
        JSON_Value* newValue = json_value_init_object();
        if (json_object_set_value(root_object, "communicationChannelHandles", newValue) != JSONSuccess)
        {
            Log_Error("Failed to set communication channel handle for session ID %s.", sessionId);
            json_value_free(newValue);
            newValue = NULL;
            goto done;
        }
        commHandles = json_value_get_object(newValue);
    }

    if (json_object_set_number(commHandles, sessionId, (double)(long)communicationChannelHandler) != JSONSuccess)
    {
        Log_Error("Failed to set communication channel handle for session ID %s.", sessionId);
        goto done;
    }

    res = ADUC_STATE_STORE_RESULT_OK;
done:
    sem_post(&state_semaphore);
    return res;
}
