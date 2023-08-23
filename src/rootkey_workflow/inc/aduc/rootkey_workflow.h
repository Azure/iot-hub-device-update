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
#include <aduc/rootkeypackage_download.h>
#include <stdbool.h>

EXTERN_C_BEGIN

ADUC_Result RootKeyWorkflow_UpdateRootKeys(const char* workflowId, const char* rootKeyPkgUrl, const char* rootKeyStoreDirPath, const char* rootKeyStorePkgFilePath, const ADUC_RootKeyPkgDownloaderInfo* downloaderInfo);

EXTERN_C_END

#endif // ROOTKEY_WORKFLOW_H
