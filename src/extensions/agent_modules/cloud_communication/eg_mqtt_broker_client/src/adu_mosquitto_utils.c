/**
 * @file adu_mosquitto_utils.c
 * @brief Implementation for utility functions for mosquitto library.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adu_mosquitto_utils.h"
#include "aduc/adu_mqtt_protocol.h"
#include "aduc/logging.h"
#include "aduc/string_c_utils.h"
#include "stddef.h"
#include "stdlib.h"
#include "string.h"
#include <aducpal/time.h> // time_t, CLOCK_REALTIME
#include <mosquitto.h> // mosquitto related functions
#include <mqtt_protocol.h>

/**
 * @brief Get the current time in seconds since the epoch.
 * @return The current time in seconds since the epoch.
 */
time_t ADUC_GetTimeSinceEpochInSeconds()
{
    struct timespec timeSinceEpoch;
    ADUCPAL_clock_gettime(CLOCK_REALTIME, &timeSinceEpoch);
    return timeSinceEpoch.tv_sec;
}

/**
 * @brief Generate a correlation ID from a time value.
 * @param[in] t The time value to use.
 * @return A string containing the correlation ID. The caller is responsible for free()'ing the returned string.
 */
char* GenerateCorrelationIdFromTime(time_t t)
{
    return ADUC_StringFormat("%ld", t);
}

/**
 * @brief Generate a correlation ID from a time value with a prefix.
 * @param[in] t The time value to use.
 * @param[in] prefix The prefix to use.
 * @return A string containing the correlation ID. The caller is responsible for free()'ing the returned string.
 */
char* GenerateCorrelationIdFromTimeWithPrefix(time_t t, const char* prefix)
{
    return ADUC_StringFormat("%s-%ld", prefix, t);
}

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
bool ADU_mosquitto_get_correlation_data(const mosquitto_property* props, char** value)
{
    if (props == NULL || value == NULL)
    {
        Log_Error("Null pointer (props=%p, value=%p)", props, value);
        return false;
    }

    *value = NULL;

    // Create a copy of Correlation Data and store it in the 'value' param.
    const mosquitto_property* p = mosquitto_property_read_string(props, MQTT_PROP_CORRELATION_DATA, value, false);
    if (p == NULL)
    {
        Log_Error("Failed to read correlation data");
        return false;
    }

    return true;
}

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
bool ADU_mosquitto_has_user_property(const mosquitto_property* props, const char* key, const char* value)
{
    bool found = false;
    while (props != NULL && !found)
    {
        char* k;
        char* v;
        const mosquitto_property* p =
            mosquitto_property_read_string_pair(props, MQTT_PROP_USER_PROPERTY, &k, &v, false);
        if (p != NULL && k != NULL && v != NULL)
        {
            found = strcmp(k, key) == 0 && strcmp(v, value) == 0;
            free(k);
            free(v);
            if (!found)
            {
                props = mosquitto_property_next(props);
            }
        }
    }
    return found;
}

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
bool ADU_mosquitto_read_user_property_string(const mosquitto_property* props, const char* key, char** value)
{
    bool found = false;
    if (props == NULL || value == NULL || key == NULL)
    {
        Log_Error("Null pointer (props=%p, value=%p, key=%p)", props, value, key);
        return found;
    }
    *value = NULL;
    while (props != NULL && !found)
    {
        char* k = NULL;
        char* v = NULL;
        const mosquitto_property* p =
            mosquitto_property_read_string_pair(props, MQTT_PROP_USER_PROPERTY, &k, &v, false);
        if (p != NULL && k != NULL && v != NULL)
        {
            found = (strcmp(k, key) == 0);
            if (found)
            {
                *value = v;
            }
            else
            {
                free(v);
            }
            free(k);

            if (!found)
            {
                props = mosquitto_property_next(props);
            }
        }
    }
    return found;
}

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
ADUC_MQTT_DISCONNECTION_CATEGORY CategorizeMQTTDisconnection(int rc)
{
    switch (rc)
    {
    case MQTT_RC_NORMAL_DISCONNECTION:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_TRANSIENT;
    case MQTT_RC_DISCONNECT_WITH_WILL_MSG:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_TRANSIENT;
    case MQTT_RC_UNSPECIFIED:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_TRANSIENT;
    case MQTT_RC_MALFORMED_PACKET:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_NON_RECOVERABLE;
    case MQTT_RC_PROTOCOL_ERROR:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_NON_RECOVERABLE;
    case MQTT_RC_IMPLEMENTATION_SPECIFIC:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_NON_RECOVERABLE;
    case MQTT_RC_NOT_AUTHORIZED:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_NON_RECOVERABLE;
    case MQTT_RC_SERVER_BUSY:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_TRANSIENT;
    case MQTT_RC_SERVER_SHUTTING_DOWN:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_NON_RECOVERABLE;
    case MQTT_RC_KEEP_ALIVE_TIMEOUT:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_TRANSIENT;
    case MQTT_RC_SESSION_TAKEN_OVER:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_NON_RECOVERABLE;
    case MQTT_RC_TOPIC_FILTER_INVALID:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_NON_RECOVERABLE;
    case MQTT_RC_TOPIC_NAME_INVALID:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_NON_RECOVERABLE;
    case MQTT_RC_RECEIVE_MAXIMUM_EXCEEDED:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_NON_RECOVERABLE;
    case MQTT_RC_TOPIC_ALIAS_INVALID:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_NON_RECOVERABLE;
    case MQTT_RC_PACKET_TOO_LARGE:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_NON_RECOVERABLE;
    case MQTT_RC_MESSAGE_RATE_TOO_HIGH:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_NON_RECOVERABLE;
    case MQTT_RC_QUOTA_EXCEEDED:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_NON_RECOVERABLE;
    case MQTT_RC_ADMINISTRATIVE_ACTION:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_NON_RECOVERABLE;
    case MQTT_RC_PAYLOAD_FORMAT_INVALID:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_NON_RECOVERABLE;
    case MQTT_RC_RETAIN_NOT_SUPPORTED:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_NON_RECOVERABLE;
    case MQTT_RC_QOS_NOT_SUPPORTED:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_NON_RECOVERABLE;
    case MQTT_RC_USE_ANOTHER_SERVER:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_TRANSIENT;
    case MQTT_RC_SERVER_MOVED:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_TRANSIENT;
    case MQTT_RC_SHARED_SUBS_NOT_SUPPORTED:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_NON_RECOVERABLE;
    case MQTT_RC_CONNECTION_RATE_EXCEEDED:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_NON_RECOVERABLE;
    case MQTT_RC_MAXIMUM_CONNECT_TIME:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_NON_RECOVERABLE;
    case MQTT_RC_SUBSCRIPTION_IDS_NOT_SUPPORTED:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_NON_RECOVERABLE;
    case MQTT_RC_WILDCARD_SUBS_NOT_SUPPORTED:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_NON_RECOVERABLE;
    default:
        return ADUC_MQTT_DISCONNECTION_CATEGORY_OTHER;
    }
}
