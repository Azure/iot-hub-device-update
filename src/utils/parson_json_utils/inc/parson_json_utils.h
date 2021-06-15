/**
 * @file parson_json_utils.h
 * @brief Implements the JSON Utility for parsing json
 *
 * @copyright Copyright (c) 2021, Microsoft Corporation.
 */

#ifndef parson_json_utils_H
#define parson_json_utils_H

#include <aduc/c_utils.h>
#include <parson.h>

EXTERN_C_BEGIN

const char* ADUC_JSON_GetStringFieldPtr(const JSON_Value* jsonValue, const char* jsonFieldName);

_Bool ADUC_JSON_GetStringField(const JSON_Value* jsonValue, const char* jsonFieldName, char** value);

_Bool ADUC_JSON_GetBooleanField(const JSON_Value* jsonValue, const char* jsonFieldName);

EXTERN_C_END

#endif