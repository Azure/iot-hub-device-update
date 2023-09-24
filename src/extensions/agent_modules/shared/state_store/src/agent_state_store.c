/**
 * @file agent_state_store.c
 * @brief Implements the Device Update agent state store functionality.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/agent_state_store.h"
#include "aduc/logging.h"
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include "parson.h"

#include "stdbool.h" // bool

// Internal state data
static JSON_Value* state_data = NULL;

// Semaphore for process synchronization
static sem_t state_semaphore;

typedef struct {
    char* externalDeviceId;
    char* deviceId;
    bool isDeviceEnrolled;
    bool isAgentInfoReported;
} ADUC_StateData;

static ADUC_StateData state = {0};  // Initialize to zero.


int ADUC_StateStore_Init(void) {
    state_data = json_value_init_object();
    if (!state_data) {
        Log_Error("Failed to initialize state data store.");
        return -1;
    }

    if (sem_init(&state_semaphore, 0, 1) != 0) {
        Log_Error("Failed to initialize semaphore.");
        json_value_free(state_data);
        state_data = NULL;
        return -1;
    }

    // Loading from the file (assuming it's "state.json")
    JSON_Value* state_data = json_parse_file("state.json");

    if (state_data == NULL) {
        state_data = json_value_init_object();
    }

    if (!state_data) {
        Log_Error("Failed to initialize state data store.");
        return -1;
    }

    JSON_Object* state_data_obj = json_value_get_object(state_data);
    state.externalDeviceId = strdup(json_object_get_string(state_data_obj, "externalDeviceId"));
    state.deviceId = strdup(json_object_get_string(state_data_obj, "deviceId"));
    state.isDeviceEnrolled = json_object_get_boolean(state_data_obj, "isDeviceEnrolled");
    state.isAgentInfoReported = json_object_get_boolean(state_data_obj, "isAgentInfoReported");

    Log_Info("State store initialized successfully.");
    return 0;
}

JSON_Value* ADUC_StateStore_GetData(const char *key) {
    sem_wait(&state_semaphore);
    JSON_Object *root_object = json_value_get_object(state_data);
    JSON_Value *result = json_object_dotget_value(root_object, key);
    sem_post(&state_semaphore);
    return result;
}

int ADUC_StateStore_SetData(const char *key, JSON_Value *value) {
    if (!key || !value) {
        Log_Error("Invalid input parameters for SetData.");
        return -1;
    }

    sem_wait(&state_semaphore);
    JSON_Object *root_object = json_value_get_object(state_data);
    if (json_object_dotset_value(root_object, key, value) != JSONSuccess) {
        Log_Error("Failed to set data for key %s.", key);
        sem_post(&state_semaphore);
        return -1;
    }
    sem_post(&state_semaphore);
    Log_Info("Data set successfully for key %s.", key);
    return 0;
}

int ADUC_StateStore_WriteToFile(const char *filename) {
    sem_wait(&state_semaphore);
    if (json_serialize_to_file(state_data, filename) != JSONSuccess) {
        Log_Error("Failed to write state data to file %s.", filename);
        sem_post(&state_semaphore);
        return -1;
    }
    sem_post(&state_semaphore);
    Log_Info("Successfully wrote state data to file %s.", filename);
    return 0;
}

void ADUC_StateStore_Terminate(void) {
    sem_wait(&state_semaphore);

    // Saving to the file
    JSON_Object* state_data_obj = json_value_get_object(state_data);

    json_object_set_string(state_data_obj, "externalDeviceId", state.externalDeviceId);
    json_object_set_string(state_data_obj, "deviceId", state.deviceId);
    json_object_set_boolean(state_data_obj, "isDeviceEnrolled", state.isDeviceEnrolled);
    json_object_set_boolean(state_data_obj, "isAgentInfoReported", state.isAgentInfoReported);

    json_serialize_to_file(state_data, "state.json");

    json_value_free(state_data);
    state_data = NULL;
    sem_post(&state_semaphore);
    sem_destroy(&state_semaphore);
    Log_Info("State store terminated successfully.");
}

// Integer data retrieval and setting
int ADUC_StateStore_GetInt(const char *key, int *out_value) {
    if (!state_data || !out_value) {
        Log_Error("Invalid input parameters for GetInt.");
        return -1;
    }

    sem_wait(&state_semaphore);
    JSON_Object *root_object = json_value_get_object(state_data);
    double result = json_object_dotget_number(root_object, key);

    if (json_object_dotget_value(root_object, key) == NULL ||
        json_value_get_type(json_object_dotget_value(root_object, key)) != JSONNumber) {
        Log_Warn("Failed to retrieve int value for key %s.", key);
        sem_post(&state_semaphore);
        return -1;
    }

    *out_value = (int)result;
    sem_post(&state_semaphore);
    Log_Info("Successfully retrieved int value for key %s.", key);
    return 0;
}

int ADUC_StateStore_SetInt(const char *key, int value) {
    return ADUC_StateStore_SetData(key, json_value_init_number(value));
}

// Unsigned integer data retrieval and setting
int ADUC_StateStore_GetUnsignedInt(const char *key, unsigned int *out_value) {
    int result;
    int status = ADUC_StateStore_GetInt(key, &result);
    if (status == 0 && result >= 0) {
        *out_value = (unsigned int)result;
        Log_Info("Successfully retrieved unsigned int value for key %s.", key);
        return 0;
    }
    Log_Warn("Failed to retrieve unsigned int value for key %s.", key);
    return -1;
}

int ADUC_StateStore_SetUnsignedInt(const char *key, unsigned int value) {
    return ADUC_StateStore_SetInt(key, (int)value);  // Note: Assumes the value is within the range of 'int'
}

// String data retrieval and setting
int ADUC_StateStore_GetString(const char *key, char **out_value) {
    if (!state_data || !out_value) {
        Log_Error("Invalid input parameters for GetString.");
        return -1;
    }

    sem_wait(&state_semaphore);
    JSON_Object *root_object = json_value_get_object(state_data);
    const char *result = json_object_dotget_string(root_object, key);

    if (!result) {
        Log_Warn("Failed to retrieve string value for key %s.", key);
        sem_post(&state_semaphore);
        return -1;
    }

    *out_value = strdup(result);  // Callers are responsible for freeing this memory.
    sem_post(&state_semaphore);
    Log_Info("Successfully retrieved string value for key %s.", key);
    return 0;
}

int ADUC_StateStore_SetString(const char *key, const char *value) {
    return ADUC_StateStore_SetData(key, json_value_init_string(value));
}

// Boolean data retrieval and setting
int ADUC_StateStore_GetBool(const char *key, int *out_value) {
    if (!state_data || !out_value) {
        Log_Error("Invalid input parameters for GetBool.");
        return -1;
    }

    sem_wait(&state_semaphore);
    JSON_Object *root_object = json_value_get_object(state_data);
    int result = json_object_dotget_boolean(root_object, key);

    if (json_object_dotget_value(root_object, key) == NULL ||
        json_value_get_type(json_object_dotget_value(root_object, key)) != JSONBoolean) {
        Log_Warn("Failed to retrieve bool value for key %s.", key);
        sem_post(&state_semaphore);
        return -1;
    }

    *out_value = result;
    sem_post(&state_semaphore);
    Log_Info("Successfully retrieved bool value for key %s.", key);
    return 0;
}

int ADUC_StateStore_SetBool(const char *key, int value) {
    return ADUC_StateStore_SetData(key, json_value_init_boolean(value));
}

const char *ADUC_StateStore_GetExternalDeviceId(void) {
    sem_wait(&state_semaphore);
    const char* value = state.externalDeviceId;
    sem_post(&state_semaphore);
    return value;
}

int ADUC_StateStore_SetExternalDeviceId(const char *externalDeviceId) {
    if (!externalDeviceId) {
        Log_Error("Invalid input for SetExternalDeviceId.");
        return -1;
    }

    sem_wait(&state_semaphore);
    free(state.externalDeviceId);
    state.externalDeviceId = strdup(externalDeviceId);
    sem_post(&state_semaphore);
    return 0;
}

const char *ADUC_StateStore_GetDeviceId(void) {
    sem_wait(&state_semaphore);
    const char* value = state.deviceId;
    sem_post(&state_semaphore);
    return value;
}

int ADUC_StateStore_SetDeviceId(const char *deviceId) {
    if (!deviceId) {
        Log_Error("Invalid input for SetDeviceId.");
        return -1;
    }

    sem_wait(&state_semaphore);
    free(state.deviceId);
    state.deviceId = strdup(deviceId);
    sem_post(&state_semaphore);
    return 0;
}

bool ADUC_StateStore_GetIsDeviceEnrolled(void) {
    sem_wait(&state_semaphore);
    bool value = state.isDeviceEnrolled;
    sem_post(&state_semaphore);
    return value;
}

int ADUC_StateStore_SetIsDeviceEnrolled(bool isDeviceEnrolled) {
    sem_wait(&state_semaphore);
    state.isDeviceEnrolled = isDeviceEnrolled;
    sem_post(&state_semaphore);
    return 0;
}

bool ADUC_StateStore_GetIsAgentInfoReported(void) {
    sem_wait(&state_semaphore);
    bool value = state.isAgentInfoReported;
    sem_post(&state_semaphore);
    return value;
}

int ADUC_StateStore_SetIsAgentInfoReported(bool isAgentInfoReported) {
    sem_wait(&state_semaphore);
    state.isAgentInfoReported = isAgentInfoReported;
    sem_post(&state_semaphore);
    return 0;
}
