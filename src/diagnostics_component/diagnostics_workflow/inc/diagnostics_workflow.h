/**
 * @file diagnostics_workflow.h
 * @brief Header for functions handling the Diagnostics Log Upload workflow
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef DIAGNOSTICS_WORKFLOW_H
#define DIAGNOSTICS_WORKFLOW_H

#include <aduc/c_utils.h>
#include <azure_c_shared_utility/strings.h>
#include <azure_c_shared_utility/vector.h>
#include <diagnostics_async_helper.h>
#include <diagnostics_result.h>
#include <stdbool.h>
#include <stdlib.h>

EXTERN_C_BEGIN

/**
 * @brief Forward declaration of JSON_Value used in Parson
 */
typedef struct json_value_t JSON_Value;

/**
 * @brief Describes each component for which to collect logs.
 */
typedef struct tagDiagnosticLogComponent
{
    STRING_HANDLE componentName; //!< Name of the component for which to collect logs
    STRING_HANDLE logPath; //!< Absolute path to the directory where the logs are stored
} DiagnosticsLogComponent;

/**
 * @brief Data structure representing the data needed for the Diagnostics Workflow
 */
typedef struct tagDiagnosticWorkflowData
{
    VECTOR_HANDLE components; //!< Vector of DiagnosticLogComponent pointers for which to collect logs
    unsigned int maxBytesToUploadPerLogPath; //!< The maximum number of bytes to upload per log file path
} DiagnosticsWorkflowData;

/**
 * @brief Initializes @p workflowData with the contents of @p filePath
 * @param[out] workflowData the workflowData structure to be intialized.
 * @param[in] filePath path to the diagnostics configuration file
 * @returns true on successful configuration, false on failure
 */
_Bool DiagnosticsWorkflow_InitFromFile(DiagnosticsWorkflowData* workflowData, const char* filePath);

/**
 * @brief Initializes @p workflowData with the contents of @p fileJsonValue
 * @param[out] workflowData the structure to be initialized
 * @param[in] fileJsonValue a JSON_Value representation of a diagnostics-config.json file
 * @returns true on successful configuration; false on failures
 */
_Bool DiagnosticsWorkflow_InitFromJSON(DiagnosticsWorkflowData* workflowData, JSON_Value* fileJsonValue);

/**
 * @brief UnInitializes @p workflowData's data members
 * @param workflowData the workflowData structure to be unintialized.
 */
void DiagnosticsWorkflow_UnInit(DiagnosticsWorkflowData* workflowData);

/**
 * @brief Returns the DiagnosticsLogComponent object @p index within @p workflowData
 * @param workflowData the DiagnosticsWorkflowData vector from which to get the DiagnosticsComponent
 * @param index the index from which to retrieve the value
 * @returns NULL if index is out of range; the DiagnosticsComponent otherwise
 */
const DiagnosticsLogComponent*
DiagnosticsWorkflow_GetLogComponentElem(const DiagnosticsWorkflowData* workflowData, unsigned int index);

/**
 * @brief Uploads the diagnostic logs described by @p workflowData
 * @param workflowData the workflowData structure describing the log components
 * @param jsonString the string from the diagnostics_interface describing where to upload the logs
 */
void DiagnosticsWorkflow_DiscoverAndUploadLogs(const DiagnosticsWorkflowData* workflowData, const char* jsonString);

EXTERN_C_END

#endif // DIAGNOSTICS_WORKFLOW_H
