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
typedef enum ADUC_STATE_STORE_RESULT_TAG
{
    ADUC_STATE_STORE_RESULT_OK = 0,
    ADUC_STATE_STORE_RESULT_ERROR = -1,
} ADUC_STATE_STORE_RESULT;

/**
 * @brief Initialize the state store.
 * @param state_file_path The path to the state file.
 * @return ADUC_STATE_STORE_RESULT_OK on success, ADUC_STATE_STORE_RESULT_ERROR on failure.
 */
ADUC_STATE_STORE_RESULT ADUC_StateStore_Initialize(const char* state_file_path);

/**
 * Retrieves the data associated with the given key.
 * @param durable Whether to search the durable or non-durable store.
 * @param key The key to search for.
 * @return The JSON value associated with the key, or NULL if not found.
 */
JSON_Value* ADUC_StateStore_GetData(bool durable, const char* key);

/**
 * Sets the data for the given key.
 * @param durable Whether to set the data in the durable or non-durable store.
 * @param key The key to set.
 * @param value The JSON value to associate with the key.
 * @return 0 on success, -1 on failure.
 */
ADUC_STATE_STORE_RESULT ADUC_StateStore_SetData(bool durable, const char* key, JSON_Value* value);

/**
 * @brief Save current state to the state file.
 */
void ADUC_StateStore_Save(void);

/**
 * @brief Deinitialize the state store. This function also save the durable states to the state file.
 */
void ADUC_StateStore_Deinitialize(void);

/**
 * @brief Get the 'ExternalDeviceId' value from the state store.
 * @remark This value is usually set by the Azure IoT Device Provisioning Service (DPS), and is used to identify the device.
 *         If DPS is not used, this value can be specified in the device's configuration file.
 * @return The value of 'ExternalDeviceId' or NULL if not found.
 */
const char* ADUC_StateStore_GetExternalDeviceId(void);

/**
 * @brief Set the 'ExternalDeviceId' value in the state store.
 * @param externalDeviceId The value to set.
 * @return ADUC_STATE_STORE_RESULT_OK on success, ADUC_STATE_STORE_RESULT_ERROR on failure.
 */
ADUC_STATE_STORE_RESULT ADUC_StateStore_SetExternalDeviceId(const char* externalDeviceId);

/**
 * @brief Set the 'IsDeviceRegistered' value in the state store.
 * @param isDeviceRegistered The value to set.
 * @return ADUC_STATE_STORE_RESULT_OK on success, ADUC_STATE_STORE_RESULT_ERROR on failure.
 */
ADUC_STATE_STORE_RESULT ADUC_StateStore_SetIsDeviceRegistered(bool isDeviceRegistered);

/**
 * @brief Get the 'IsDeviceRegistered' value in the state store.
 * @return The value of 'IsDeviceRegistered' or false if not found.
 */
bool ADUC_StateStore_GetIsDeviceRegistered();

/**
 * @brief Get the recommended polling interval for device registration state.
 * @return The recommended polling interval in seconds.
 */
int ADUC_StateStore_GetDeviceRegistrationStatePollIntervalSeconds();

/**
 * @brief Get the Device Update service MQTT broker hostname.
 */
const char* ADUC_StateStore_GetMQTTBrokerHostname(void);

/**
 * @brief Set the Device Update service MQTT broker hostname.
 * @param hostname The value to set.
 * @return ADUC_STATE_STORE_RESULT_OK on success, ADUC_STATE_STORE_RESULT_ERROR on failure.
 */
ADUC_STATE_STORE_RESULT ADUC_StateStore_SetMQTTBrokerHostname(const char* hostname);

/**
 * @brief Get the 'DeviceId' value from the state store.
 */
const char* ADUC_StateStore_GetDeviceId(void);

/**
 * @brief Set the 'DeviceId' value in the state store.
 * @param deviceId The value to set.
 * @return ADUC_STATE_STORE_RESULT_OK on success, ADUC_STATE_STORE_RESULT_ERROR on failure.
 */
ADUC_STATE_STORE_RESULT ADUC_StateStore_SetDeviceId(const char* deviceId);

/**
 * @brief Get the DU service instance name from the state store.
 */
const char* ADUC_StateStore_GetDeviceUpdateServiceInstance(void);

/**
 * @brief Set the DU service instance name in the state store.
 * @param instanceName The value to set.
 * @return ADUC_STATE_STORE_RESULT_OK on success, ADUC_STATE_STORE_RESULT_ERROR on failure.
 */
ADUC_STATE_STORE_RESULT ADUC_StateStore_SetDeviceUpdateServiceInstance(const char* instanceName);

/**
 * @brief Get a boolean value indicating whether the device is enrolled with the Device Update service.
 * @return true if the device is enrolled, false otherwise.
 */
bool ADUC_StateStore_GetIsDeviceEnrolled(void);

/**
 * @brief Set a boolean value indicating whether the device is enrolled with the Device Update service.
 * @remark This value should be set by the ADU Enrollment State Management module.
 * @param isDeviceEnrolled The value to set.
 * @return ADUC_STATE_STORE_RESULT_OK on success, ADUC_STATE_STORE_RESULT_ERROR on failure.
 */
ADUC_STATE_STORE_RESULT ADUC_StateStore_SetIsDeviceEnrolled(bool isDeviceEnrolled);

/**
 * @brief Get a boolean value indicating whether the DU agent information has been reported to, and acknowledged by the Device Update service.
 * @return true if the agent information has been reported, false otherwise.
 */
bool ADUC_StateStore_GetIsAgentInfoReported(void);

/**
 * @brief Set a boolean value indicating whether the DU agent information has been reported to, and acknowledged by the Device Update service.
 * @remark This value should be set by the ADU Agent Information Management module.
 * @param isAgentInfoReported The value to set.
 * @return ADUC_STATE_STORE_RESULT_OK on success, ADUC_STATE_STORE_RESULT_ERROR on failure.
 */
ADUC_STATE_STORE_RESULT ADUC_StateStore_SetIsAgentInfoReported(bool isAgentInfoReported);

/**
 * @brief Get a pointer to the communication channel handle.
 * @param sessionId The session ID  of the communication channel.
 * @return A pointer to the communication channel handle, or NULL if not found.
 */
const void* ADUC_StateStore_GetCommunicationChannelHandle(const char* sessionId);

/**
 * @brief Set the communication channel handle.
 * @param sessionId The session ID of the communication channel.
 * @param communicationChannelHandler The communication channel handle to set.
 * @return ADUC_STATE_STORE_RESULT_OK on success, ADUC_STATE_STORE_RESULT_ERROR on failure.
 */
ADUC_STATE_STORE_RESULT
ADUC_StateStore_SetCommunicationChannelHandle(const char* sessionId, const void* communicationChannelHandler);

ADUC_STATE_STORE_RESULT ADUC_StateStore_GetInt(bool durable, const char* key, int* out_value);
ADUC_STATE_STORE_RESULT ADUC_StateStore_SetInt(bool durable, const char* key, int value);
ADUC_STATE_STORE_RESULT ADUC_StateStore_GetUnsignedInt(bool durable, const char* key, unsigned int* out_value);
ADUC_STATE_STORE_RESULT ADUC_StateStore_SetUnsignedInt(bool durable, const char* key, unsigned int value);
ADUC_STATE_STORE_RESULT ADUC_StateStore_GetString(bool durable, const char* key, char** out_value);
ADUC_STATE_STORE_RESULT ADUC_StateStore_SetString(bool durable, const char* key, const char* value);
ADUC_STATE_STORE_RESULT ADUC_StateStore_GetBool(bool durable, const char* key, int* out_value);
ADUC_STATE_STORE_RESULT ADUC_StateStore_SetBool(bool durable, const char* key, int value);
ADUC_STATE_STORE_RESULT ADUC_StateStore_GetJsonValue(bool durable, const char* key, void** out_value);
ADUC_STATE_STORE_RESULT ADUC_StateStore_SetJsonValue(bool durable, const char* key, void* value);

EXTERN_C_END

#endif // ADUC_MQTT_STATE_STORE_H
