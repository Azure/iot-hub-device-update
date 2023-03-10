/**
 * @file diagnostics_devicename.h
 * @brief Header file describing the methods for setting the static global device name for use in the diagnostic component
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ROOTKEY_WORKFLOW_H
#define ROOTKEY_WORKFLOW_H

#include <aduc/c_utils.h>
#include <aduc/result.h>
#include <stdbool.h>

EXTERN_C_BEGIN

bool RootKeyWorkflow_NeedToUpdateRootKeys(void);

ADUC_Result RootKeyWorkflow_UpdateRootKeys(const char* workflowId, const char* rootKeyPkgUrl);

EXTERN_C_END

#endif // ROOTKEY_WORKFLOW_H
