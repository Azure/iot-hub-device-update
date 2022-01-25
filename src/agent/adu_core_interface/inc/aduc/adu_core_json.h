/**
 * @file adu_core_json.h
 * @brief Defines types and methods required to parse JSON from the urn:azureiot:AzureDeviceUpdateCore:1 interface.
 *
 * Note that the types in this file must be kept in sync with the AzureDeviceUpdateCore interface.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_ADU_CORE_JSON_H
#define ADUC_ADU_CORE_JSON_H

#include "parson.h"
#include <aduc/c_utils.h>
#include <aduc/types/update_content.h>
#include <aduc/types/workflow.h>
#include <stdbool.h>

EXTERN_C_BEGIN
//
// Methods
//

struct json_value_t;
typedef struct json_value_t JSON_Value;

struct tagADUC_FileEntity;
struct tagADUC_UpdateId;

_Bool ADUC_Json_ValidateManifest(const JSON_Value* updateActionJson);

JSON_Value* ADUC_Json_GetRoot(const char* updateActionJsonString);

_Bool ADUC_Json_GetUpdateAction(const JSON_Value* updateActionJson, unsigned* updateAction);

_Bool ADUC_Json_GetInstalledCriteria(const JSON_Value* updateActionJson, char** installedCriteria);

_Bool ADUC_Json_GetUpdateType(const JSON_Value* updateActionJson, char** updateTypeStr);

_Bool ADUC_Json_GetFiles(
    const JSON_Value* updateActionJson, unsigned int* fileCount, struct tagADUC_FileEntity** files);

EXTERN_C_END

#endif // ADUC_ADU_CORE_JSON_H
