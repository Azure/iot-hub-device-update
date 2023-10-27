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
#include <mosquitto.h>

/**
 * @brief The length of a correlation ID. (including null terminator)
 */
#define CORRELATION_ID_LENGTH 37

/**
 * @brief The length of a correlation ID without hyphens. (including null terminator)
 */
#define CORRELATION_ID_LENGTH_WITHOUT_HYPHENS 33

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
 * @brief Check if the correlation data in the MQTT properties matches the provided correlation ID.
 * @param[in] props Pointer to the MQTT properties from which to retrieve the correlation data.
 * @param[in] correlationId The correlation ID to check against.
 * @return `true` if the correlation data matches the provided correlation ID; otherwise, `false`.
 */
bool ADU_are_correlation_ids_matching(const mosquitto_property* props, const char* correlationId);

#endif
