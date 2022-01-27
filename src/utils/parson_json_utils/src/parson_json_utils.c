/**
 * @file parson_json_utils.c
 * @brief Provides set of functions for reading json
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "parson_json_utils.h"

#include <aduc/logging.h>
#include <aduc/string_c_utils.h>
#include <azure_c_shared_utility/crt_abstractions.h>
#include <azure_c_shared_utility/strings.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/**
 * @brief Returns the pointer to the @p jsonFieldName from the JSON_Value
 * @details if @p jsonValue is free this value will become invalid
 * @param jsonValue The JSON Value
 * @param jsonFieldName The name of the JSON field to get
 * @return on success a pointer to the string value of @p jsonFieldName, on failure NULL
 *
 */
const char* ADUC_JSON_GetStringFieldPtr(const JSON_Value* jsonValue, const char* jsonFieldName)
{
    JSON_Object* object = json_value_get_object(jsonValue);

    if (object == NULL)
    {
        return NULL;
    }

    return json_object_get_string(object, jsonFieldName);
}

/**
 * @brief Gets a boolean field from the a JSON Value.
 *
 * @param jsonValue The JSON Value.
 * @param jsonFieldName The name of the JSON field to get.
 * @return the boolean value of the jsonFieldName, returns false on fail and logs error
 */
_Bool ADUC_JSON_GetBooleanField(const JSON_Value* jsonValue, const char* jsonFieldName)
{
    JSON_Object* object = json_value_get_object(jsonValue);

    if (object == NULL)
    {
        return false;
    }

    int result = json_object_get_boolean(object, jsonFieldName);
    if (result == -1)
    {
        Log_Error("Cannot get json field name %s, default to false.", jsonFieldName);
        return false;
    }
    return result;
}

/**
 * @brief Gets a string field from the a JSON Value.
 *
 * @param jsonValue The JSON Value.
 * @param jsonFieldName The name of the JSON field to get.
 * @param value The buffer to fill with the value from the JSON field. Caller must call free().
 * @return _Bool true if call succeeded. false otherwise.
 */
_Bool ADUC_JSON_GetStringField(const JSON_Value* jsonValue, const char* jsonFieldName, char** value)
{
    _Bool succeeded = false;

    *value = NULL;

    JSON_Object* root_object = json_value_get_object(jsonValue);
    if (root_object == NULL)
    {
        goto done;
    }

    const char* value_val = json_object_get_string(root_object, jsonFieldName);
    if (value_val == NULL)
    {
        goto done;
    }

    if (mallocAndStrcpy_s(value, value_val) != 0)
    {
        goto done;
    }

    succeeded = true;

done:
    if (!succeeded)
    {
        if (*value != NULL)
        {
            free(*value);
            *value = NULL;
        }
    }

    return succeeded;
}

/**
 * @brief Gets the value of the field @p jsonFieldName from @p jsonObj and allocates and stores it in @p value
 * @details Caller is responsible for calling free() on @p value when finished
 * @param jsonObj the json object to ge the field value from
 * @param jsonFieldName field to get the string value for
 * @param value The buffer to fill with the value from the JSON field. Caller must call free()
 * @returns true on success; false on failure
 */
_Bool ADUC_JSON_GetStringFieldFromObj(const JSON_Object* jsonObj, const char* jsonFieldName, char** value)
{
    if (jsonObj == NULL || jsonFieldName == NULL)
    {
        return false;
    }

    const char* fieldValue = json_object_get_string(jsonObj, jsonFieldName);

    if (mallocAndStrcpy_s(value, fieldValue) != 0)
    {
        return false;
    }

    return true;
}

/**
 * @brief Gets the integer representation of a the value of @p jsonFieldName from @p jsonValue and assigns it to @p value
 *
 * @details All values in json are doubles this function only returns true if the value read is a whole,
 * @param jsonValue value to extract the @p jsonFieldName value from
 * @param value the parameter to store @p jsonFieldName's value in
 * @returns true on success; false on failure
 */
_Bool ADUC_JSON_GetUnsignedIntegerField(const JSON_Value* jsonValue, const char* jsonFieldName, unsigned int* value)
{
    if (jsonValue == NULL || jsonFieldName == NULL)
    {
        return false;
    }

    _Bool succeeded = false;
    double val = 0;
    unsigned int castVal = 0;

    JSON_Object* jsonObj = json_value_get_object(jsonValue);

    if (jsonObj == NULL)
    {
        goto done;
    }

    // Note: cannot determine failure in this call as 0 is a valid return, always assume succeeded at this point
    val = json_object_get_number(jsonObj, jsonFieldName);

    // Note: check value is not negative and is a whole number for safe casting to an unsigned int
    if (val < 0 || ((int)val) != val)
    {
        goto done;
    }

    castVal = (unsigned int)val;

    succeeded = true;
done:

    *value = castVal;

    return succeeded;
}
