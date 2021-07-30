/**
 * @file connection_string_utils.h
 * @brief connection string utilities.
 *
 * @copyright Copyright (c) 2021, Microsoft Corp.
 */

#include <azure_c_shared_utility/connection_string_parser.h>
#include <azure_c_shared_utility/map.h>

#include "aduc/c_utils.h"
#include "aduc/connection_string_utils.h"
#include "aduc/string_c_utils.h"

//
// Connection string keys
//

/**
 * Example connection string
 *
 * HostName=some-iot-device.io;DeviceId=somedeviceid;SharedAccessKey=11111111111111111111111111111111111;GatewayHostName=255.255.255.254
 */

/**
 * @brief Key for the GatewayHostName in the connection string
 */
#define CONNECTION_STRING_GATEWAY_HOSTNAME_KEY "GatewayHostName"

/**
 * @brief Determines if the given key exists in the connection string.
 * @param[in] connectionString The connection string from the connection info.
 * @param[in] key The key in question.
 * @return Whether the key was found in the connection string.
 */
_Bool ConnectionStringUtils_DoesKeyExist(const char* connectionString, const char* key)
{
    _Bool keyExists = false;

    MAP_HANDLE map = connectionstringparser_parse_from_char(connectionString);
    if (map == NULL)
    {
        goto done;
    }

    _Bool tmp = false;
    MAP_RESULT result = Map_ContainsKey(map, key, &tmp);
    if (result != MAP_OK)
    {
        goto done;
    }

    keyExists = tmp;

done:
    Map_Destroy(map);

    return keyExists;
}

/**
 * @brief Gets the value for the given key out of the connection string.
 * @param[in] connectionString The connection string from the connection info.
 * @param[in] key The key for the value in question.
 * @param[out] A copy of the value for the key allocated with malloc, or NULL if not found.
 * @return true on success; false otherwise.
 */
_Bool ConnectionStringUtils_GetValue(const char* connectionString, const char* key, char** value)
{
    _Bool result = false;
    MAP_HANDLE map = NULL;

    if (value == NULL)
    {
        goto done;
    }

    map = connectionstringparser_parse_from_char(connectionString);
    if (map == NULL)
    {
        goto done;
    }

    const char* mapValue = Map_GetValueFromKey(map, key);
    if (mapValue == NULL)
    {
        result = true;
        *value = NULL;
    }
    else
    {
        char* resultValue = NULL;
        if (mallocAndStrcpy_s(&resultValue, mapValue) != 0)
        {
            goto done;
        }

        result = true;
        *value = resultValue;
    }

done:
    Map_Destroy(map);

    return result;
}

/**
 * @brief Determines if the connection string indicates nested edge connectivity.
 * @param[in] connectionString the connection string from the connection info.
 * @return Whether it is nested edge connectivity.
 */
_Bool ConnectionStringUtils_IsNestedEdge(const char* connectionString)
{
    return ConnectionStringUtils_DoesKeyExist(connectionString, CONNECTION_STRING_GATEWAY_HOSTNAME_KEY);
}
