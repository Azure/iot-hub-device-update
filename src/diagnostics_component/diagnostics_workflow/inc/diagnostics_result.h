/**
 * @file Diagnostics_Result.h
 * @brief Defines result values for the Diagnostics Interface
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef DIAGNOSTICS_RESULT_H
#define DIAGNOSTICS_RESULT_H

#include <aduc/c_utils.h>

#include <stdbool.h>

EXTERN_C_BEGIN

/**
 * @brief Enum defining the values that the DiagnosticsInterface can return to the Diagnostics Service
 */
typedef enum tagDiagnostics_Result
{
    Diagnostics_Result_NoSasCredential = -7, //!< Cloud to device message contains no sas credential
    Diagnostics_Result_NoOperationId = -6, // !< Cloud to device message contains no operation id
    Diagnostics_Result_NoDiagnosticsComponents = -5, // !< Diagnostics configuration doesn't contain any components
    Diagnostics_Result_BadCredential = -4, // !< The storageSasCredential sent to the device is improperly formed
    Diagnostics_Result_ContainerCreateFailed = -3, //!< Unable to create the container for some log upload
    Diagnostics_Result_UploadFailed = -2, //!< Uploading the files failed for some component
    Diagnostics_Result_NoLogsFound = -1, //!< No logs where found for a component
    Diagnostics_Result_Failure = 0, //!< DiagnosticsWorkflow failed in a generic, non workflow related way
    Diagnostics_Result_Success = 200, //!< Upload of all component logs was successful.
} Diagnostics_Result;

/**
 * @brief Returns the string version of @p result
 * @param result Diagnostics_Result value to return the string representation of
 * @returns the string name of @p result, <Unknown> otherwise
 */
const char* DiagnosticsResult_ToString(Diagnostics_Result result);

EXTERN_C_END

#endif // DIAGNOSTICS_RESULT_H
