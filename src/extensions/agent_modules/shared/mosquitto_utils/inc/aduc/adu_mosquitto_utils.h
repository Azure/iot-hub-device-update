/**
 * @file adu_mosquitto_utils.h
 * @brief A header file for utility functions for mosquitto library.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef __ADU_MOSQUITTO_UTILS_H__
#define __ADU_MOSQUITTO_UTILS_H__

#include "aducpal/time.h"
#include <aduc/mqtt_broker_common.h> // ADUC_Common_Response_User_Properties
#include <mosquitto.h>

/**
 * @brief Enumeration representing MQTT disconnection categories.
 */
typedef enum ADUC_MQTT_DISCONNECTION_CATEGORY_TAG
{
    /**
     * Transient category: Disconnects that might be recoverable by reattempting the connection.
     */
    ADUC_MQTT_DISCONNECTION_CATEGORY_TRANSIENT,

    /**
     * Non-recoverable category: Disconnects that are not likely to be recoverable and require action.
     */
    ADUC_MQTT_DISCONNECTION_CATEGORY_NON_RECOVERABLE,

    /**
     * Other category: Disconnects that do not fall into the above categories.
     */
    ADUC_MQTT_DISCONNECTION_CATEGORY_OTHER
} ADUC_MQTT_DISCONNECTION_CATEGORY;

/**
 * @brief Categorize MQTT disconnection result.
 *
 * This function categorizes an MQTT disconnection result into three categories based on the
 * updated ADUC_MQTT_DISCONNECTION_CATEGORY enumeration:
 * - ADUC_MQTT_DISCONNECTION_CATEGORY_TRANSIENT: Disconnects that might be recoverable by
 *   reattempting the connection.
 * - ADUC_MQTT_DISCONNECTION_CATEGORY_NON_RECOVERABLE: Disconnects that are not likely to be
 *   recoverable and require action.
 * - ADUC_MQTT_DISCONNECTION_CATEGORY_OTHER: Disconnects that do not fall into the above categories.
 *
 * @param rc The MQTT disconnection result code.
 * @return An enumeration value representing the category of the disconnection result.
 */
ADUC_MQTT_DISCONNECTION_CATEGORY CategorizeMQTTDisconnection(int rc);

/**
 * @brief Generate a correlation ID from a time value.
 * @param[in] t The time value to use.
 * @return A string containing the correlation ID. The caller is responsible for free()'ing the returned string.
 */
char* GenerateCorrelationIdFromTime(time_t t);

/**
 * @brief Retrieves the correlation data from MQTT properties.
 *
 * This function attempts to read the correlation data from the provided MQTT properties.
 * On success, the retrieved value is stored in the passed 'value' parameter.
 *
 * @param[in] props Pointer to the MQTT properties from which to retrieve the correlation data.
 * @param[out] value Pointer to a char pointer where the retrieved correlation data string will be stored.
 *                   The caller is responsible for freeing the allocated memory using `free`.
 *
 * @return `true` if correlation data was successfully retrieved; otherwise, `false`.
 *
 * @note If the function fails to retrieve the correlation data or if any of the input parameters is NULL,
 *       an error message will be logged.
 */
bool ADU_mosquitto_get_correlation_data(const mosquitto_property* props, char** value);

/**
 * @brief Retrieve the value of a specific user property from an MQTT v5 property list.
 *
 * This function searches the provided MQTT v5 property list for a user property with the
 * specified key. If found, it populates the value parameter with the corresponding value.
 *
 * @param[in] props Pointer to the head of the MQTT v5 property list.
 * @param[in] key The key of the user property to search for.
 * @param[out] value Pointer to the location where the function will store the corresponding
 *                   value for the given key, if found. This value must be free'd by the caller.
 *
 * @return True if the property list contains the user property with the specified key,
 *         and the value parameter has been populated. Otherwise, it returns false.
 *
 * @note If there are multiple properties with the same key, this function will populate
 * the value parameter with the value of the first key it encounters.
 *
 * @warning If any of the input pointers are NULL, the function will log an error and return false.
 */
bool ADU_mosquitto_read_user_property_string(const mosquitto_property* props, const char* key, char** value);

/**
 * @brief Reads the user property utf-8 string value and attempts to parse it as an int32_t.
 *
 * @param props The props list.
 * @param key The property name to search for.
 * @param outValue The resultant int32 value.
 * @return true When property name key is found and string value in its entirety is successfully parsed as int32 number.
 * @details Will set contents of outValue pointer to parsed int32 when return success, and sets it to 0 when failed.
 */
bool ADU_mosquitto_read_user_property_as_int32(const mosquitto_property* props, const char* key, int32_t* outValue);

/**
 * @brief Check if a specific user property exists within a property list.
 *
 * This function iterates through an MQTT v5 property list searching for
 * a specific user property with the given key and value.
 *
 * @param[in] props Pointer to the head of the MQTT v5 property list.
 * @param[in] key The key of the user property to search for.
 * @param[in] value The expected value for the given key.
 *
 * @return True if the property list contains the user property with the specified key and value, otherwise false.
 *
 * @note If there are multiple properties with the same key, this function will
 * return true upon finding the first key-value match.
 */
bool ADU_mosquitto_has_user_property(const mosquitto_property* props, const char* key, const char* value);

/**
 * @brief Adds a user property name value pair to the props property list.
 *
 * @param props The props list. If it's the first time adding a user property, then ensure that *props is assigned NULL.
 * @param name The name of the user property, e.g. "mt"
 * @param value The value of the user property, e.g. "ainfo_req"
 * @return true If succeeded in adding the user property utf-8 pair.
 * @details if *props is NULL, then on success, *props will not be null. Caller will need to free by calling ADU_mosquitto_free_properties(&props)
 */
bool ADU_mosquitto_add_user_property(mosquitto_property** props, const char* name, const char* value);

/**
 * @brief Adds the correlation data property to the property list.
 *
 * @param props The address of the property list.
 * @param correlationData The correlation data string.
 * @return true on success.
 */
bool ADU_mosquitto_set_correlation_data_property(mosquitto_property** props, const char* correlationData);

/**
 * @brief Frees the property list created using ADU_mosquitto_add_user_property()
 *
 * @param props The address of mosquitto_property pointer property list.
 */
void ADU_mosquitto_free_properties(mosquitto_property** props);

/**
 * @brief Check if the correlation data in the MQTT properties matches the provided correlation ID.
 * @param[in] props Pointer to the MQTT properties from which to retrieve the correlation data.
 * @param[in] correlationId The correlation ID to check against.
 * @param[out] outCorrelationData Optional. If not NULL, will set to a string with the value of mqtt correlation-data property.
 * @param[out] outCorrelationDataByteLen Optional. If not NULL, will set to byte length of the mqtt correlation-data property.
 * @return true if the correlation data matches the provided correlation ID; otherwise, false.
 * @remark
 */
bool ADU_are_correlation_ids_matching(const mosquitto_property* props, const char* correlationId, char** outCorrelationData, size_t* outCorrelationDataByteLen);

/**
 * @brief Parses and validates the MQTT response topic user properties common to ADU responst topics.
 *
 * @param props The MQTT response property list.
 * @param expectedMsgType The expected value for "mt" message type user property.
 * @param outRespUserProps The common response user properties structure that will receive side-effects after parse and validation.
 * @return true When parse and validation succeeds.
 */
bool ParseAndValidateCommonResponseUserProperties(const mosquitto_property* props, const char* expectedMsgType, ADUC_Common_Response_User_Properties* outRespUserProps);

/**
 * @brief prints mosquitto property for debugging.
 *
 * @param properties The mosquito properties
 * @return int 0 on success.
 */
int json_print_properties(const mosquitto_property* properties);

/**
 * @brief Generate a GUID  7d28dcd5-175c-46ed-b3bb-a557d278da56
 *
 * @param with_hyphens Whether to include hyphens in the GUID.
 * @param buffer Where to store identifier.
 * @param buffer_cch Number of characters in @p buffer.
 *
 */
bool ADUC_generate_correlation_id(bool with_hyphens, char* buffer, size_t buffer_cch);

#endif
