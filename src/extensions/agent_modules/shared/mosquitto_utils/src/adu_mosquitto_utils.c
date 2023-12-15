/**
 * @file adu_mosquitto_utils.c
 * @brief Implementation for utility functions for mosquitto library.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adu_mosquitto_utils.h"

#include <aduc/logging.h>
#include <aduc/parse_num.h> // ADUC_parse_int32_t
#include <aduc/string_c_utils.h> // ADUC_StringFormat
#include <aducpal/time.h> // time_t, CLOCK_REALTIME
#include <du_agent_sdk/agent_common.h> // IGNORED_PARAMETER
#include <mosquitto.h> // mosquitto related functions
#include <mqtt_protocol.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>

// keep this last to avoid interfering with system headers
#include <aduc/aduc_banned.h>

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
        return false;
    }

    *value = NULL;

    // Create a copy of Correlation Data and store it in the 'value' param.
    const mosquitto_property* p = mosquitto_property_read_string(props, MQTT_PROP_CORRELATION_DATA, value, false);
    if (p == NULL)
    {
        return false;
    }

    return true;
}

/**
 * @brief Check if the correlation data in the MQTT properties matches the provided correlation ID.
 * @param[in] props Pointer to the MQTT properties from which to retrieve the correlation data.
 * @param[in] correlationId The correlation ID to check against.
 * @param[out] outCorrelationData Optional. If not NULL, will set to a string with the value of mqtt correlation-data property.
 * @param[out] outCorrelationDataByteLen Optional. If not NULL, will set to byte length of the mqtt correlation-data property.
 * @return `true` if the correlation data matches the provided correlation ID; otherwise, `false`.
 */
bool ADU_are_correlation_ids_matching(const mosquitto_property* props, const char* correlationId, char** outCorrelationData, size_t* outCorrelationDataByteLen)
{
    char* cid = NULL;
    uint16_t cidLen = 0;
    size_t n = 0;
    bool res = false;

    if (props == NULL || IsNullOrEmpty(correlationId))
    {
        goto done;
    }

    // Check if correlation data matches the latest enrollment message.
    const mosquitto_property* p = mosquitto_property_read_binary(props, MQTT_PROP_CORRELATION_DATA, (void**)&cid, &cidLen, false);
    if (p == NULL || cid == NULL)
    {
        goto done;
    }

    if (outCorrelationData != NULL)
    {
        if (ADUC_AllocAndStrCopyN(outCorrelationData, cid, cidLen) != 0)
        {
            goto done;
        }
    }

    if (outCorrelationDataByteLen != NULL)
    {
        *outCorrelationDataByteLen = (size_t)cidLen;
    }

    n = (size_t)cidLen;
    // Check if the correlation data matches the latest enrollment message.
    if (cid == NULL || strncmp(cid, correlationId, n) != 0)
    {
        goto done;
    }
    res = true;
done:
    free(cid);
    return res;
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
 * @brief Adds a user property name value pair to the props property list.
 *
 * @param props The props list. If it's the first time adding a user property, then ensure that *props is assigned NULL.
 * @param name The name of the user property, e.g. "mt"
 * @param value The value of the user property, e.g. "ainfo_req"
 * @return true If succeeded in adding the user property utf-8 pair.
 * @details if *props is NULL, then on success, *props will not be null. Caller will need to free by calling ADU_mosquitto_free_properties(&props)
 */
bool ADU_mosquitto_add_user_property(mosquitto_property** props, const char* name, const char* value)
{
    int err = mosquitto_property_add_string_pair(props, MQTT_PROP_USER_PROPERTY, name, value);

    if (err == MOSQ_ERR_SUCCESS)
    {
        return true;
    }

    switch(err)
    {
    case MOSQ_ERR_INVAL: // - if identifier is invalid, if name or value is NULL, or if proplist is NULL
        Log_Error("Fail MOSQ_ERR_INVAL(%d)", err);
        break;

    case MOSQ_ERR_NOMEM: // - on out of memory
        Log_Error("Fail MOSQ_ERR_NOMEM(%d)", err);
        break;

    case MOSQ_ERR_MALFORMED_UTF8: // - if name or value are not valid UTF-8.
        Log_Error("Fail MOSQ_ERR_MALFORMED_UTF8(%d)", err);
        break;

    default:
        Log_Error("Fail Unknown(%d)", err);
        break;
    }

    return false;
}

/**
 * @brief Adds the correlation data property to the property list.
 *
 * @param props The address of the property list.
 * @param correlationData The correlation data string.
 * @return true on success.
 */
bool ADU_mosquitto_set_correlation_data_property(mosquitto_property** props, const char* correlationData)
{
    int err = mosquitto_property_add_binary(props, MQTT_PROP_CORRELATION_DATA, (void*)correlationData, (uint16_t)strlen(correlationData));

    if ( err == MOSQ_ERR_SUCCESS)
    {
        return true;
    }

    switch(err)
    {
    case MOSQ_ERR_INVAL: // - if identifier is invalid, if name or value is NULL, or if proplist is NULL
        Log_Error("Fail MOSQ_ERR_INVAL(%d) - props[%p] correlationData[%p]", err, props, correlationData);
        break;

    case MOSQ_ERR_NOMEM: // - on out of memory
        Log_Error("Fail MOSQ_ERR_NOMEM(%d)", err);
        break;

    case MOSQ_ERR_MALFORMED_UTF8: // - if name or value are not valid UTF-8.
        Log_Error("Fail MOSQ_ERR_MALFORMED_UTF8(%d)", err);
        break;

    default:
        Log_Error("Fail Unknown(%d)", err);
        break;
    }


    return false;
}

/**
 * @brief Frees the property list created using ADU_mosquitto_add_user_property()
 *
 * @param props The address of mosquitto_property pointer property list.
 */
void ADU_mosquitto_free_properties(mosquitto_property** props)
{
    mosquitto_property_free_all(props);
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
 * @brief Reads the user property utf-8 string value and attempts to parse it as an int32_t.
 *
 * @param props The props list.
 * @param key The property name to search for.
 * @param outValue The resultant int32 value.
 * @return true When property name key is found and string value in its entirety is successfully parsed as int32 number.
 * @details Will set contents of outValue pointer to parsed int32 when return success, and sets it to 0 when failed.
 */
bool ADU_mosquitto_read_user_property_as_int32(const mosquitto_property* props, const char* key, int32_t* outValue)
{
    bool succeeded = false;
    char* val = NULL;
    int32_t parsed = 0;

    if (ADU_mosquitto_read_user_property_string(props, key, &val))
    {
        succeeded = ADUC_parse_int32_t(val, &parsed);
        free(val);
    }

    *outValue = succeeded ? parsed : 0;
    return succeeded;
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

/**
 * @brief Parses and validates the MQTT response topic user properties common to ADU responst topics.
 *
 * @param props The MQTT response property list.
 * @param expectedMsgType The expected value for "mt" message type user property.
 * @param outRespUserProps The common response user properties structure that will receive side-effects after parse and validation.
 * @return true When parse and validation succeeds.
 */
bool ParseAndValidateCommonResponseUserProperties(const mosquitto_property* props, const char* expectedMsgType, ADUC_Common_Response_User_Properties* outRespUserProps)
{
    bool success = false;

    char* msg_type = NULL;
    int32_t pid = 0;
    int32_t resultcode = 0;
    int32_t extendedresultcode = 0;

    // Parse

    if (!ADU_mosquitto_read_user_property_string(props, "mt", &msg_type))
    {
        Log_Error("Fail parse 'mt' from user props");
        goto done;
    }

    if (!ADU_mosquitto_read_user_property_as_int32(props, "pid", &pid))
    {
        Log_Error("Fail parse 'pid' from user props");
        goto done;
    }

    if (!ADU_mosquitto_read_user_property_as_int32(props, "resultcode", &resultcode))
    {
        Log_Error("Fail parse 'resultcode' from user props");
        goto done;
    }

    if (!ADU_mosquitto_read_user_property_as_int32(props, "extendedresultcode", &extendedresultcode))
    {
        Log_Error("Fail parse 'extendedresultcode' from user props");
        goto done;
    }

    // Validate

    if (pid != 1)
    {
        char* pid_str = NULL;
        ADU_mosquitto_read_user_property_string(props, "pid", &pid_str);

        Log_Error("Invalid 'pid' user property: %s", pid_str);
        free(pid_str);

        goto done;
    }

    if (strcmp(msg_type, expectedMsgType) != 0)
    {
        Log_Error("Invalid 'mt' user property: '%s' expected '%s'", msg_type, expectedMsgType);
        goto done;
    }

    Log_Debug("Successful Parse + Validate: '%s' user properties - pid[%d] resultcode[%d] extendedresultcode[%d]", msg_type, pid, resultcode, extendedresultcode);

    // Now, assign to outRespUserProps post validation.
    outRespUserProps->pid = pid;
    outRespUserProps->resultcode = resultcode;
    outRespUserProps->extendedresultcode = extendedresultcode;

    success = true;
done:
    free(msg_type);

    return success;
}

int json_print_properties(const mosquitto_property* properties)
{
    int identifier;
    uint8_t i8value = 0;
    uint16_t i16value = 0;
    uint32_t i32value = 0;
    char *strname = NULL, *strvalue = NULL;
    char* binvalue = NULL;
    const mosquitto_property* prop = NULL;

    for (prop = properties; prop != NULL; prop = mosquitto_property_next(prop))
    {
        identifier = mosquitto_property_identifier(prop);
        switch (identifier)
        {
        case MQTT_PROP_PAYLOAD_FORMAT_INDICATOR:
            mosquitto_property_read_byte(prop, MQTT_PROP_PAYLOAD_FORMAT_INDICATOR, &i8value, false);
            break;

        case MQTT_PROP_MESSAGE_EXPIRY_INTERVAL:
            mosquitto_property_read_int32(prop, MQTT_PROP_MESSAGE_EXPIRY_INTERVAL, &i32value, false);
            break;

        case MQTT_PROP_CONTENT_TYPE:
        case MQTT_PROP_RESPONSE_TOPIC:
            mosquitto_property_read_string(prop, identifier, &strvalue, false);
            if (strvalue == NULL)
                return MOSQ_ERR_NOMEM;
            free(strvalue);
            strvalue = NULL;
            break;

        case MQTT_PROP_CORRELATION_DATA:
            mosquitto_property_read_binary(prop, MQTT_PROP_CORRELATION_DATA, (void**)&binvalue, &i16value, false);
            if (binvalue == NULL)
                return MOSQ_ERR_NOMEM;
            free(binvalue);
            binvalue = NULL;
            break;

        case MQTT_PROP_SUBSCRIPTION_IDENTIFIER:
            mosquitto_property_read_varint(prop, MQTT_PROP_SUBSCRIPTION_IDENTIFIER, &i32value, false);
            break;

        case MQTT_PROP_TOPIC_ALIAS:
            mosquitto_property_read_int16(prop, MQTT_PROP_TOPIC_ALIAS, &i16value, false);
            break;

        case MQTT_PROP_USER_PROPERTY:
            mosquitto_property_read_string_pair(prop, MQTT_PROP_USER_PROPERTY, &strname, &strvalue, false);
            if (strname == NULL || strvalue == NULL)
                return MOSQ_ERR_NOMEM;

            free(strvalue);

            free(strname);
            strname = NULL;
            strvalue = NULL;
            break;
        }
    }

    return MOSQ_ERR_SUCCESS;
}

/**
 * @brief Generate a GUID  7d28dcd5-175c-46ed-b3bb-a557d278da56
 *
 * @param with_hyphens Whether to include hyphens in the GUID.
 * @param buffer Where to store identifier.
 * @param buffer_cch Number of characters in @p buffer.
 *
 */
bool ADUC_generate_correlation_id(bool with_hyphens, char* buffer, size_t buffer_cch)
{
    uuid_t bin_uuid;
    char uuid_str[37];  // 36 bytes + NUL

    if (buffer_cch < 33 || (with_hyphens && buffer_cch < 37))
    {
        return false;
    }

    // Generate a UUID
    uuid_generate(bin_uuid);
    // Convert the binary UUID to a string
    uuid_unparse(bin_uuid, uuid_str);

    if (with_hyphens)
    {
        if (strcpy(buffer, uuid_str) == NULL)
        {
            return false;
        }
    }
    else
    {
        int j = 0;
        for (int i = 0; i < 36; i++)
        {
            if (uuid_str[i] != '-')
            {
                buffer[j++] = uuid_str[i];
            }
        }
        buffer[j] = '\0';
    }

    return true;
}
