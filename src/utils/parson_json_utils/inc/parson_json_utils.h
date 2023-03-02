/**
 * @file parson_json_utils.h
 * @brief Implements the JSON Utility for parsing json
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef PARSON_JSON_UTILS_H
#define PARSON_JSON_UTILS_H

#include <aduc/c_utils.h>
#include <parson.h>
#include <stdbool.h>

EXTERN_C_BEGIN

//
// JSON_Value Helper Utils
//
const char* ADUC_JSON_GetStringFieldPtr(const JSON_Value* jsonValue, const char* jsonFieldName);

bool ADUC_JSON_GetStringField(const JSON_Value* jsonValue, const char* jsonFieldName, char** value);

bool ADUC_JSON_GetBooleanField(const JSON_Value* jsonValue, const char* jsonFieldName);

bool ADUC_JSON_GetUnsignedIntegerField(const JSON_Value* jsonValue, const char* jsonFieldName, unsigned int* value);

//
// JSON_Object Helper Utils
//
bool ADUC_JSON_GetStringFieldFromObj(const JSON_Object* jsonObj, const char* jsonFieldName, char** value);

EXTERN_C_END

#endif // PARSON_JSON_UTILS_H
