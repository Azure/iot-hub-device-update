/**
 * @file diagnostics_config_utils.c
 * @brief Implementation for functions handling the Diagnostic config
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "diagnostics_config_utils.h"
#include <aduc/logging.h>
#include <parson_json_utils.h> // for ADUC_JSON_GetUnsignedIntegerField
#include <stdlib.h> // for free

/**
 * @brief Fieldname for the array of log components in the Diagnostics JSON Config File
 */
#define DIAGNOSTICS_CONFIG_FILE_LOG_COMPONENTS_FIELDNAME "logComponents"

/**
 * @brief Fieldname for the name of the component having its logs collected in the Diagnostics JSON Config File
 */
#define DIAGNOSTICS_CONFIG_FILE_COMPONENT_FIELDNAME_COMPONENTNAME "componentName"

/**
 * @brief Fieldname for the location of logs on the device for a component in the Diagnostics JSON Config File
 */
#define DIAGNOSTICS_CONFIG_FILE_COMPONENT_FIELDNAME_LOGPATH "logPath"

/**
 * @brief Fieldname for the maximum number of bytes to upload per diagnostics workflow
 */
#define DIAGNOSTICS_CONFIG_FILE_FIELDNAME_MAXKILOBYTESTOUPLOADPERLOGPATH "maxKilobytesToUploadPerLogPath"

/**
 * @brief Maximum number of kilobytes allowed to be uploaded per log path
 */
#define DIAGNOSTICS_MAX_KILOBYTES_PER_LOG_PATH 100000 /* 1000 KB or 100 MB */

/**
 * Expected Diagnostics Config file format:
 * {
        "logComponents":[
            {
                "componentName":"DU",
                "logPath":"/var/logs/adu/"
            },
            {
                "componentName":"DO",
                "logPath":"/var/cache/do/"
            },
            ...
        ],
        "maxKilobytesToUploadPerLogPath":5
    }
 */

/**
 * @brief initializes a log component using a JSON_Object representing the component in the config file
 * @param component the componentObj to intitialize
 * @param componentObj JSON Object with the associated components for the log component
 * @returns true on success; false on failure
 */
static bool
DiagnosticsConfigUtils_LogComponentInitFromObj(DiagnosticsLogComponent* component, JSON_Object* componentObj)
{
    if (componentObj == NULL || component == NULL)
    {
        return false;
    }

    memset(component, 0, sizeof(*component));

    bool succeeded = false;
    char* componentName = NULL;
    char* logPath = NULL;

    if (!ADUC_JSON_GetStringFieldFromObj(
            componentObj, DIAGNOSTICS_CONFIG_FILE_COMPONENT_FIELDNAME_COMPONENTNAME, &componentName))
    {
        goto done;
    }

    if (!ADUC_JSON_GetStringFieldFromObj(componentObj, DIAGNOSTICS_CONFIG_FILE_COMPONENT_FIELDNAME_LOGPATH, &logPath))
    {
        goto done;
    }

    component->componentName = STRING_construct(componentName);

    if (component->componentName == NULL)
    {
        goto done;
    }

    component->logPath = STRING_construct(logPath);

    if (component->logPath == NULL)
    {
        goto done;
    }

    succeeded = true;

done:

    if (!succeeded)
    {
        DiagnosticsConfigUtils_LogComponentUninit(component);
    }

    free(componentName);
    free(logPath);

    return succeeded;
}

/**
 * @brief Initializes @p workflowData with the contents of @p fileJsonValue
 * @param workflowData the structure to be initialized
 * @param fileJsonValue a JSON_Value representation of a diagnostics-config.json file
 * @returns true on successful configuration; false on failures
 */
bool DiagnosticsConfigUtils_InitFromJSON(DiagnosticsWorkflowData* workflowData, JSON_Value* fileJsonValue)
{
    if (workflowData == NULL || fileJsonValue == NULL)
    {
        return false;
    }

    bool succeeded = false;

    memset(workflowData, 0, sizeof(*workflowData));

    JSON_Object* fileJsonObj = json_value_get_object(fileJsonValue);

    if (fileJsonObj == NULL)
    {
        goto done;
    }

    unsigned int maxKilobytesToUploadPerLogPath = 0;

    if (!ADUC_JSON_GetUnsignedIntegerField(
            fileJsonValue,
            DIAGNOSTICS_CONFIG_FILE_FIELDNAME_MAXKILOBYTESTOUPLOADPERLOGPATH,
            &maxKilobytesToUploadPerLogPath))
    {
        Log_Warn("DiagnosticsConfigUtils_Init failed to retrieve maxKilobytesPerLogPath from diagnostics config");
        goto done;
    }

    if (maxKilobytesToUploadPerLogPath < 1)
    {
        Log_Warn(
            "DiagnosticsConfigUtils_Init maxKilobytesPerLogPath config set to invalid value: %i",
            maxKilobytesToUploadPerLogPath);
        goto done;
    }

    if (maxKilobytesToUploadPerLogPath > DIAGNOSTICS_MAX_KILOBYTES_PER_LOG_PATH)
    {
        maxKilobytesToUploadPerLogPath = DIAGNOSTICS_MAX_KILOBYTES_PER_LOG_PATH;
    }

    workflowData->maxBytesToUploadPerLogPath = maxKilobytesToUploadPerLogPath * 1024;

    JSON_Array* componentArray = json_object_get_array(fileJsonObj, DIAGNOSTICS_CONFIG_FILE_LOG_COMPONENTS_FIELDNAME);

    if (componentArray == NULL)
    {
        goto done;
    }

    const size_t numComponents = json_array_get_count(componentArray);

    if (numComponents == 0)
    {
        goto done;
    }

    workflowData->components = VECTOR_create(sizeof(DiagnosticsLogComponent));

    if (workflowData->components == NULL)
    {
        goto done;
    }

    for (size_t i = 0; i < numComponents; ++i)
    {
        JSON_Object* componentObj = json_array_get_object(componentArray, i);

        DiagnosticsLogComponent logComponent = {};

        if (!DiagnosticsConfigUtils_LogComponentInitFromObj(&logComponent, componentObj))
        {
            goto done;
        }

        if (VECTOR_push_back(workflowData->components, &logComponent, 1) != 0)
        {
            goto done;
        }
    }

    succeeded = true;
done:

    if (!succeeded)
    {
        Log_Error("DiagnosticsConfigUtils_Init failed");
        DiagnosticsConfigUtils_UnInit(workflowData);
    }

    return succeeded;
}

/**
 * @brief Initializes @p workflowData with the contents of @p filePath
 * @param workflowData the workflowData structure to be intialized.
 * @param filePath path to the diagnostics configuration file
 * @returns true on successful configuration, false on failure
 */
bool DiagnosticsConfigUtils_InitFromFile(DiagnosticsWorkflowData* workflowData, const char* filePath)
{
    JSON_Value* fileJsonValue = NULL;

    if (workflowData == NULL || filePath == NULL)
    {
        return false;
    }

    fileJsonValue = json_parse_file(filePath);

    if (fileJsonValue == NULL)
    {
        return false;
    }

    bool succeeded = DiagnosticsConfigUtils_InitFromJSON(workflowData, fileJsonValue);

    json_value_free(fileJsonValue);

    return succeeded;
}

/**
 * @brief Returns the DiagnosticsLogComponent object @p index within @p workflowData
 * @param workflowData the DiagnosticsWorkflowData vector from which to get the DiagnosticsComponent
 * @param index the index from which to retrieve the value
 * @returns NULL if index is out of range; the DiagnosticsComponent otherwise
 */
const DiagnosticsLogComponent*
DiagnosticsConfigUtils_GetLogComponentElem(const DiagnosticsWorkflowData* workflowData, unsigned int index)
{
    if (workflowData == NULL || index >= VECTOR_size(workflowData->components))
    {
        return NULL;
    }

    return (const DiagnosticsLogComponent*)VECTOR_element(workflowData->components, index);
}

/**
 * @brief Uninitializes a logComponent pointer
 * @param logComponent pointer to the logComponent whose members will be freed
 */
void DiagnosticsConfigUtils_LogComponentUninit(DiagnosticsLogComponent* logComponent)
{
    if (logComponent == NULL)
    {
        return;
    }

    STRING_delete(logComponent->componentName);
    logComponent->componentName = NULL;

    STRING_delete(logComponent->logPath);
    logComponent->logPath = NULL;
}

/**
 * @brief UnInitializes @p workflowData's data members
 * @param workflowData the workflowData structure to be unintialized.
 */
void DiagnosticsConfigUtils_UnInit(DiagnosticsWorkflowData* workflowData)
{
    if (workflowData == NULL)
    {
        return;
    }

    if (workflowData->components != NULL)
    {
        const size_t numComponents = VECTOR_size(workflowData->components);

        for (size_t i = 0; i < numComponents; i++)
        {
            DiagnosticsLogComponent* logComponent =
                (DiagnosticsLogComponent*)VECTOR_element(workflowData->components, i);

            DiagnosticsConfigUtils_LogComponentUninit(logComponent);
        }

        VECTOR_destroy(workflowData->components);
    }

    memset(workflowData, 0, sizeof(*workflowData));
}
