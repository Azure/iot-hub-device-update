/**
 * @file diagnostics_result.c
 * @brief Implementation for diagnostics_Result.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "diagnostics_result.h"

/**
 * @brief Returns the string version of @p result
 * @param result Diagnostics_Result value to return the string representation of
 * @returns the string name of @p result, <Unknown> otherwise
 */
const char* DiagnosticsResult_ToString(Diagnostics_Result result)
{
    switch (result)
    {
    case Diagnostics_Result_NoSasCredential:
        return "NoSasCredential";
    case Diagnostics_Result_NoOperationId:
        return "NoOperationId";
    case Diagnostics_Result_NoDiagnosticsComponents:
        return "NoDiagnosticsComponents";
    case Diagnostics_Result_BadCredential:
        return "BadCredential";
    case Diagnostics_Result_ContainerCreateFailed:
        return "ContainerCreateFailed";
    case Diagnostics_Result_UploadFailed:
        return "UploadFailed";
    case Diagnostics_Result_NoLogsFound:
        return "NoLogsFound";
    case Diagnostics_Result_Failure:
        return "Failure";
    case Diagnostics_Result_Success:
        return "Success";
    }

    return "<Unknown>";
}
