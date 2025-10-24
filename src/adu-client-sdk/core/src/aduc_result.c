/**
 * @file aduc_result.c
 * @brief Implementation of Azure Device Update Core SDK Result utilities
 * 
 * @copyright Copyright (C) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See LICENSE file in the project root for license information.
 */

#include "aduc/result.h"
#include <string.h>
#include <stdlib.h>

/**
 * @brief Initialize result details structure.
 */
void ADUC_Result_Init(
    ADUC_ResultDetails* details,
    ADUC_Result_t resultCode,
    ADUC_Result_t extendedResultCode,
    const char* resultDetails,
    const char* stepId)
{
    if (details == NULL)
    {
        return;
    }

    memset(details, 0, sizeof(ADUC_ResultDetails));
    details->resultCode = resultCode;
    details->extendedResultCode = extendedResultCode;
    
    if (resultDetails != NULL)
    {
        details->resultDetails = strdup(resultDetails);
    }
    
    if (stepId != NULL)
    {
        details->stepId = strdup(stepId);
    }
}

/**
 * @brief Create a success result.
 */
void ADUC_Result_SetSuccess(ADUC_ResultDetails* details, const char* description)
{
    ADUC_Result_Init(details, ADUC_Result_Success, 0, description, NULL);
}

/**
 * @brief Create a failure result.
 */
void ADUC_Result_SetFailure(
    ADUC_ResultDetails* details, 
    ADUC_Result_t resultCode, 
    const char* description)
{
    ADUC_Result_Init(details, resultCode, 0, description, NULL);
}

/**
 * @brief Check if result indicates success.
 */
bool ADUC_Result_IsSuccess(const ADUC_ResultDetails* details)
{
    return (details != NULL) && (details->resultCode == ADUC_Result_Success);
}

/**
 * @brief Check if result indicates failure.
 */
bool ADUC_Result_IsFailure(const ADUC_ResultDetails* details)
{
    return (details == NULL) || (details->resultCode != ADUC_Result_Success);
}

/**
 * @brief Get result code as string.
 */
const char* ADUC_Result_ToString(ADUC_Result_t resultCode)
{
    switch (resultCode)
    {
        case ADUC_Result_Success: return "Success";
        case ADUC_Result_Failure: return "Failure";
        case ADUC_Result_Failure_Cancelled: return "Cancelled";
        case ADUC_Result_Failure_InvalidArgument: return "Invalid Argument";
        case ADUC_Result_Failure_OutOfMemory: return "Out of Memory";
        case ADUC_Result_Failure_NotSupported: return "Not Supported";
        case ADUC_Result_Failure_NotImplemented: return "Not Implemented";
        case ADUC_Result_Failure_Timeout: return "Timeout";
        case ADUC_Result_Failure_FileNotFound: return "File Not Found";
        case ADUC_Result_Failure_FileAccess: return "File Access Denied";
        case ADUC_Result_Failure_DiskFull: return "Disk Full";
        case ADUC_Result_Failure_FileCorrupt: return "File Corrupt";
        case ADUC_Result_Failure_FileExists: return "File Already Exists";
        case ADUC_Result_Failure_NetworkError: return "Network Error";
        case ADUC_Result_Failure_ConnectionTimeout: return "Connection Timeout";
        case ADUC_Result_Failure_HostNotFound: return "Host Not Found";
        case ADUC_Result_Failure_Unauthorized: return "Unauthorized";
        case ADUC_Result_Failure_Forbidden: return "Forbidden";
        case ADUC_Result_Failure_ContentNotFound: return "Content Not Found";
        case ADUC_Result_Failure_InvalidManifest: return "Invalid Manifest";
        case ADUC_Result_Failure_InvalidContent: return "Invalid Content";
        case ADUC_Result_Failure_VerificationFailed: return "Verification Failed";
        case ADUC_Result_Failure_HashMismatch: return "Hash Mismatch";
        case ADUC_Result_Failure_SignatureInvalid: return "Invalid Signature";
        case ADUC_Result_Failure_InstallFailed: return "Installation Failed";
        case ADUC_Result_Failure_UninstallFailed: return "Uninstallation Failed";
        case ADUC_Result_Failure_ApplyFailed: return "Apply Failed";
        case ADUC_Result_Failure_RollbackFailed: return "Rollback Failed";
        case ADUC_Result_Failure_RestartRequired: return "Restart Required";
        case ADUC_Result_Failure_IncompatibleVersion: return "Incompatible Version";
        case ADUC_Result_Failure_ConfigError: return "Configuration Error";
        case ADUC_Result_Failure_MissingConfig: return "Missing Configuration";
        case ADUC_Result_Failure_InvalidConfig: return "Invalid Configuration";
        case ADUC_Result_Failure_SystemError: return "System Error";
        case ADUC_Result_Failure_ServiceUnavailable: return "Service Unavailable";
        case ADUC_Result_Failure_PermissionDenied: return "Permission Denied";
        case ADUC_Result_Failure_ResourceBusy: return "Resource Busy";
        case ADUC_Result_Failure_ExtensionError: return "Extension Error";
        case ADUC_Result_Failure_HandlerNotFound: return "Handler Not Found";
        case ADUC_Result_Failure_HandlerLoadFailed: return "Handler Load Failed";
        case ADUC_Result_Failure_InterfaceError: return "Interface Error";
        default: return "Unknown Error";
    }
}

/**
 * @brief Copy result details.
 */
void ADUC_Result_Copy(ADUC_ResultDetails* dest, const ADUC_ResultDetails* src)
{
    if (dest == NULL || src == NULL)
    {
        return;
    }

    dest->resultCode = src->resultCode;
    dest->extendedResultCode = src->extendedResultCode;
    
    if (src->resultDetails != NULL)
    {
        dest->resultDetails = strdup(src->resultDetails);
    }
    else
    {
        dest->resultDetails = NULL;
    }
    
    if (src->stepId != NULL)
    {
        dest->stepId = strdup(src->stepId);
    }
    else
    {
        dest->stepId = NULL;
    }
}

/**
 * @brief Free any allocated memory in result details.
 */
void ADUC_Result_Free(ADUC_ResultDetails* details)
{
    if (details == NULL)
    {
        return;
    }

    if (details->resultDetails != NULL)
    {
        free((void*)details->resultDetails);
        details->resultDetails = NULL;
    }
    
    if (details->stepId != NULL)
    {
        free((void*)details->stepId);
        details->stepId = NULL;
    }
    
    details->resultCode = 0;
    details->extendedResultCode = 0;
}