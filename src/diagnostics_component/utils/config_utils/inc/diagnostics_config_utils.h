#ifndef DIAGNOSTICS_CONFIG_UTILS_H
#define DIAGNOSTICS_CONFIG_UTILS_H

#include <aduc/c_utils.h>
#include <azure_c_shared_utility/strings.h>
#include <azure_c_shared_utility/vector.h>
#include <parson.h>

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
 * @brief Initializes @p workflowData with the contents of @p fileJsonValue
 * @param workflowData the structure to be initialized
 * @param fileJsonValue a JSON_Value representation of a diagnostics-config.json file
 * @returns true on successful configuration; false on failures
 */
bool DiagnosticsConfigUtils_InitFromJSON(DiagnosticsWorkflowData* workflowData, JSON_Value* fileJsonValue);

/**
 * @brief Initializes @p workflowData with the contents of @p filePath
 * @param workflowData the workflowData structure to be intialized.
 * @param filePath path to the diagnostics configuration file
 * @returns true on successful configuration, false on failure
 */
bool DiagnosticsConfigUtils_InitFromFile(DiagnosticsWorkflowData* workflowData, const char* filePath);

/**
 * @brief UnInitializes @p workflowData's data members
 * @param workflowData the workflowData structure to be unintialized.
 */
void DiagnosticsConfigUtils_UnInit(DiagnosticsWorkflowData* workflowData);

/**
 * @brief Returns the DiagnosticsLogComponent object @p index within @p workflowData
 * @param workflowData the DiagnosticsWorkflowData vector from which to get the DiagnosticsComponent
 * @param index the index from which to retrieve the value
 * @returns NULL if index is out of range; the DiagnosticsComponent otherwise
 */
const DiagnosticsLogComponent*
DiagnosticsConfigUtils_GetLogComponentElem(const DiagnosticsWorkflowData* workflowData, unsigned int index);

/**
 * @brief Uninitializes a logComponent pointer
 * @param logComponent pointer to the logComponent whose members will be freed
 */
void DiagnosticsConfigUtils_LogComponentUninit(DiagnosticsLogComponent* logComponent);

EXTERN_C_END

#endif // #define DIAGNOSTICS_CONFIG_UTILS_H
