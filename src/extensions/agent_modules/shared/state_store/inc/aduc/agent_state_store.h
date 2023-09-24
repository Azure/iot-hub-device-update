/**
 * @file agent_state_store.h
 * @brief Header file for Device Update agent state store functions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_MQTT_STATE_STORE_H
#define ADUC_MQTT_STATE_STORE_H

#include "parson.h"
#include "stdbool.h"
#include "aduc/c_utils.h"

EXTERN_C_BEGIN

/**
 * Initialize the state store. Must be called before any other functions.
 * @return 0 on success, -1 on failure.
 */
int ADUC_StateStore_Init(void);

/**
 * Retrieves the data associated with the given key.
 * @param key The key to search for.
 * @return The JSON value associated with the key, or NULL if not found.
 */
JSON_Value* ADUC_StateStore_GetData(const char *key);

/**
 * Sets the data for the given key.
 * @param key The key to set.
 * @param value The JSON value to associate with the key.
 * @return 0 on success, -1 on failure.
 */
int ADUC_StateStore_SetData(const char *key, JSON_Value *value);

/**
 * Writes the current in-memory data store to a file.
 * @param filename The file to write to.
 * @return 0 on success, -1 on failure.
 */
int ADUC_StateStore_WriteToFile(const char *filename);

/**
 * Cleans up resources, particularly the semaphore.
 */
void ADUC_StateStore_Terminate(void);

const char *ADUC_StateStore_GetExternalDeviceId(void);
int ADUC_StateStore_SetExternalDeviceId(const char *externalDeviceId);

const char *ADUC_StateStore_GetDeviceId(void);
int ADUC_StateStore_SetDeviceId(const char *deviceId);

bool ADUC_StateStore_GetIsDeviceEnrolled(void);
int ADUC_StateStore_SetIsDeviceEnrolled(bool isDeviceEnrolled);

bool ADUC_StateStore_GetIsAgentInfoReported(void);
int ADUC_StateStore_SetIsAgentInfoReported(bool isAgentInfoReported);


void* ADUC_StateStore_GetCommunicationChannelHandler(void);
int ADUC_StateStore_SetCommunicationChannelHandler(void* communicationChannelHandler);

EXTERN_C_END

#endif // ADUC_MQTT_STATE_STORE_H
