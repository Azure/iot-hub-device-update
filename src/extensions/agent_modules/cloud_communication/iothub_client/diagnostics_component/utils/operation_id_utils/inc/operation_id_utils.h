/**
 * @file operation_id_utils.h
 * @brief Header file for utilities for operations such as storing the operation id or checking if the operation has already been completed
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <aduc/c_utils.h>
#include <azure_c_shared_utility/strings.h>
#include <parson.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef OPERATION_ID_UTILS_H
#    define OPERATION_ID_UTILS_H

EXTERN_C_BEGIN

bool OperationIdUtils_OperationIsComplete(const char* serviceMsg);

bool OperationIdUtils_StoreCompletedOperationId(const char* operationId);

EXTERN_C_END

#endif //  OPERATION_ID_UTILS_H
