/**
 * @file operation_id_utils.cpp
 * @brief Implementation file for utilities for operations such as storing the operation id or checking if the operation has already been completed
 * @details This utility is required to prevent the processing of duplicate requests coming down from the service after a restart or a connection refresh
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "operation_id_utils.h"
#include <aduc/logging.h>
#include <aduc/system_utils.h>
#include <diagnostics_interface.h>
#include <parson_json_utils.h>

/**
 * @brief Maximum characters an operation-id can be. Used to do a bounded strcmp on operation-ids
 */
#define MAX_OPERATION_ID_CHARS 256

/**
 * @brief Json field name for the last completed operation-id within the completed operation-id json file
 */
#define COMPLETED_OPERATION_ID_JSON_FIELDNAME "lastCompletedOperationId"

/**
 * @brief Checks the DIAGNOSTICS_COMPLETED_OPERATION_FILE_PATH for the last completed operation-id and compares it against the operation-id within @p serviceMsg
 * @param serviceMsg the message from the service that contains the operation-id
 * @return true if the operation-id has already been run; false otherwise
 */
bool OperationIdUtils_OperationIsComplete(const char* serviceMsg)
{
    bool alreadyCompleted = false;

    char completedOperationId[MAX_OPERATION_ID_CHARS + 1]; // +1 for the null terminator

    JSON_Value* serviceMsgJsonValue = NULL;

    if (serviceMsg == NULL)
    {
        goto done;
    }

    if (ADUC_SystemUtils_ReadStringFromFile(
            DIAGNOSTICS_COMPLETED_OPERATION_FILE_PATH, completedOperationId, ARRAY_SIZE(completedOperationId))
        != 0)
    {
        Log_Info("Operation ID could not be read from the file because it does not exist");
        goto done;
    }

    serviceMsgJsonValue = json_parse_string(serviceMsg);

    if (serviceMsgJsonValue == NULL)
    {
        goto done;
    }

    JSON_Object* serviceMsgJsonObj = json_value_get_object(serviceMsgJsonValue);

    if (serviceMsgJsonObj == NULL)
    {
        goto done;
    }

    const char* requestOperationId = json_object_get_string(serviceMsgJsonObj, DIAGNOSTICSITF_FIELDNAME_OPERATIONID);

    if (requestOperationId == NULL)
    {
        goto done;
    }

    if (strcmp(requestOperationId, completedOperationId) == 0)
    {
        alreadyCompleted = true;
    }

done:

    json_value_free(serviceMsgJsonValue);

    return alreadyCompleted;
}

/**
 * @brief Stores @p operationId in DIAGNOSTICS_COMPLETED_OPERATION_FILE_PATH so it can be checked later on
 * @details This function is NOT thread safe.
 * @param operationId the operation-id to be stored
 * @return true if the operation-id is stored; false otherwise
 */
bool OperationIdUtils_StoreCompletedOperationId(const char* operationId)
{
    if (operationId == NULL)
    {
        return false;
    }

    if (ADUC_SystemUtils_WriteStringToFile(DIAGNOSTICS_COMPLETED_OPERATION_FILE_PATH, operationId) != 0)
    {
        Log_Info("Failed to record diagnostcis operation-id: %s", operationId);
        return false;
    }

    return true;
}
