/**
 * @file adu_types.c
 * @brief Implements helper functions related to ConnectionInfo
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adu_types.h"
#include "aduc/connection_string_utils.h"
#include "aduc/logging.h"
#include "aduc/string_c_utils.h"
#include "aduc/types/update_content.h"

#include "parson.h"

#include <stdlib.h>
#include <string.h>

#include <azure_c_shared_utility/crt_abstractions.h> // for mallocAndStrcpy_s

/**
 * @brief Returns the string associated with @p connType
 * @param connType ADUC_ConnType to be stringified
 * @returns if the ADUC_ConnType exists then the string version of the value is returned, "" otherwise
 */
const char* ADUC_ConnType_ToString(const ADUC_ConnType connType)
{
    switch (connType)
    {
    case ADUC_ConnType_NotSet:
        return "ADUC_ConnType_NotSet";
    case ADUC_ConnType_Device:
        return "ADUC_ConnType_Device";
    case ADUC_ConnType_Module:
        return "ADUC_ConnType_Module";
    }

    return "<Unknown>";
}
/**
 * @brief DeAllocates the ADUC_ConnectionInfo object
 * @param info the ADUC_ConnectionInfo object to be de-allocated
 */
void ADUC_ConnectionInfo_DeAlloc(ADUC_ConnectionInfo* info)
{
    free(info->connectionString);
    info->connectionString = NULL;

    free(info->certificateString);
    info->certificateString = NULL;

    free(info->opensslEngine);
    info->opensslEngine = NULL;

    free(info->opensslPrivateKey);
    info->opensslPrivateKey = NULL;

    info->authType = ADUC_AuthType_NotSet;
    info->connType = ADUC_ConnType_NotSet;
}

/**
 * @brief Checks if the UpdateId is valid
 * @param updateId updateId to check
 * @returns True if it is valid, false if not
 */
bool ADUC_IsValidUpdateId(const ADUC_UpdateId* updateId)
{
    return !(
        updateId == NULL || IsNullOrEmpty(updateId->Provider) || IsNullOrEmpty(updateId->Name)
        || IsNullOrEmpty(updateId->Version));
}

/**
 * @brief Free the UpdateId and its content.
 * @param updateId a pointer to an updateId struct to be freed
 */
void ADUC_UpdateId_UninitAndFree(ADUC_UpdateId* updateId)
{
    if (updateId == NULL)
    {
        return;
    }

    free(updateId->Provider);
    updateId->Provider = NULL;

    free(updateId->Name);
    updateId->Name = NULL;

    free(updateId->Version);
    updateId->Version = NULL;

    free(updateId);
}

/**
 * @brief Convert UpdateState to string representation.
 *
 * @param updateState State to convert.
 * @return const char* String representation.
 */
const char* ADUCITF_StateToString(ADUCITF_State updateState)
{
    switch (updateState)
    {
    case ADUCITF_State_None:
        return "None";
    case ADUCITF_State_Idle:
        return "Idle";
    case ADUCITF_State_DownloadStarted:
        return "DownloadStarted";
    case ADUCITF_State_DownloadSucceeded:
        return "DownloadSucceeded";
    case ADUCITF_State_BackupStarted:
        return "BackupStarted";
    case ADUCITF_State_BackupSucceeded:
        return "BackupSucceeded";
    case ADUCITF_State_InstallStarted:
        return "InstallStarted";
    case ADUCITF_State_InstallSucceeded:
        return "InstallSucceeded";
    case ADUCITF_State_RestoreStarted:
        return "RestoreStarted";
    case ADUCITF_State_ApplyStarted:
        return "ApplyStarted";
    case ADUCITF_State_DeploymentInProgress:
        return "DeploymentInProgress";
    case ADUCITF_State_Failed:
        return "Failed";
    }

    return "<Unknown>";
}

/**
 * @brief Convert UpdateAction to string representation.
 *
 * @param updateAction Action to convert.
 * @return const char* String representation.
 */
const char* ADUCITF_UpdateActionToString(ADUCITF_UpdateAction updateAction)
{
    switch (updateAction)
    {
    case ADUCITF_UpdateAction_Invalid_Download:
        return "Invalid (Download)";
    case ADUCITF_UpdateAction_Invalid_Install:
        return "Invalid (Install)";
    case ADUCITF_UpdateAction_Invalid_Apply:
        return "Invalid (Apply)";
    case ADUCITF_UpdateAction_ProcessDeployment:
        return "ProcessDeployment";
    case ADUCITF_UpdateAction_Cancel:
        return "Cancel";
    case ADUCITF_UpdateAction_Undefined:
        return "Undefined";
    }

    return "<Unknown>";
}
