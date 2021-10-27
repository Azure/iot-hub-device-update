/**
 * @file simulator_handler.cpp
 * @brief Implementation of ContentHandler API for update content simulator.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 */
#include "aduc/simulator_handler.hpp"
#include "aduc/adu_core_exports.h"
#include "aduc/logging.h"
#include "aduc/string_c_utils.h"
#include "aduc/string_utils.hpp"
#include "aduc/system_utils.h"
#include "aduc/workflow_data_utils.h"
#include "aduc/workflow_utils.h"
#include "parson.h"

#include <azure_c_shared_utility/strings.h> // STRING_*

#include <algorithm>
#include <fstream>
#include <functional>
#include <memory>
#include <sstream>
#include <string>

#include <dirent.h>

#define SIMULATOR_DATA_FILE "du-simulator-data.json"

EXTERN_C_BEGIN

/**
 * @brief Instantiates an Simulator Update Content Handler
 */
ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel)
{
    ADUC_Logging_Init(logLevel, "simulator-handler");
    Log_Info("Instantiating a Simulator Update Content Handler");
    try
    {
        return SimulatorHandlerImpl::CreateContentHandler();
    }
    catch (const std::exception& e)
    {
        const char* what = e.what();
        Log_Error("Unhandled std exception: %s", what);
    }
    catch (...)
    {
        Log_Error("Unhandled exception");
    }

    return nullptr;
}

EXTERN_C_END

// Forward declarations.
static ADUC_Result CancelApply(const char* logFolder);

/**
 * @brief Creates a new SimulatorHandlerImpl object and casts to a ContentHandler.
 * Note that there is no way to create a SimulatorHandlerImpl directly.
 *
 * @return ContentHandler* SimulatorHandlerImpl object as a ContentHandler.
 */
ContentHandler* SimulatorHandlerImpl::CreateContentHandler()
{
    return new SimulatorHandlerImpl();
}

/**
 * @brief Get the simulator data file path.
 *
 * @return char* A buffer contains file path. Caller must call free() once done.
 */
char* GetSimulatorDataFilePath()
{
    return ADUC_StringFormat("%s/%s", ADUC_SystemUtils_GetTemporaryPathName(), SIMULATOR_DATA_FILE);
}

/**
 * @brief Load data from simulator data file.
 *        This function calls GetSimulatorDataFilePath() to retrieve the data file path.
 *
 * @return JSON_Object A json object containing simulator data.
 *         Caller must free the wrapping JSON_Value* object to free the memory.
 */
JSON_Object* ReadDataFile()
{
    auto dataFilePath = GetSimulatorDataFilePath();
    JSON_Value* root_value = json_parse_file(dataFilePath);
    if (root_value == nullptr)
    {
        Log_Info("Cannot read datafile: %s", dataFilePath);
        goto done;
    }

done:
    free(dataFilePath);
    return json_value_get_object(root_value);
}

/**
 * @brief Mock implementation of download action.
 * @return ADUC_Result Return result from simulator data file if specified.
 *         Otherwise, return ADUC_Result_Download_Success.
 */
ADUC_Result SimulatorHandlerImpl::Download(const ADUC_WorkflowData* workflowData)
{
    ADUC_Result result = { .ResultCode = ADUC_Result_Download_Success };
    ADUC_WorkflowHandle handle = workflowData->WorkflowHandle;
    ADUC_WorkflowHandle childHandle = nullptr;

    bool useBundleFiles = true;
    auto fileCount = static_cast<unsigned int>(workflow_get_bundle_updates_count(handle));
    if (fileCount == 0)
    {
        useBundleFiles = false;
        fileCount = static_cast<unsigned int>(workflow_get_update_files_count(handle));
    }

    STRING_HANDLE leafManifestFile = nullptr;
    STRING_HANDLE childId = nullptr;
    JSON_Object* downloadResult = nullptr;

    JSON_Object* data = ReadDataFile();
    if (data == nullptr)
    {
        Log_Info("No simulator data file provided, returning default result code...");
        result = { .ResultCode = ADUC_Result_Download_Success };
        goto done;
    }

    // Simulate download for each file in the workflowData.
    downloadResult = json_value_get_object(json_object_get_value(data, "download"));

    for (size_t i = 0; i < fileCount; i++)
    {
        ADUC_FileEntity* entity = nullptr;
        result = { .ResultCode = ADUC_Result_Download_Success };

        bool fileEntityOk = useBundleFiles ? workflow_get_bundle_updates_file(handle, i, &entity)
                                           : workflow_get_update_file(handle, i, &entity);

        if (!fileEntityOk || entity == nullptr)
        {
            result = { .ResultCode = ADUC_Result_Failure,
                       .ExtendedResultCode = ADUC_ERC_BUNDLE_CONTENT_HANDLER_GET_FILE_ENTITY_FAILURE };
            goto done;
        }

        Log_Info("Downloading file#%d (targetFileName:%s).", i, entity->TargetFilename);

        JSON_Object* resultForFile =
            json_value_get_object(json_object_get_value(downloadResult, entity->TargetFilename));

        if (resultForFile == nullptr)
        {
            Log_Info("No matching results for file '%s', fallback to catch-all result", entity->TargetFilename);

            resultForFile = json_value_get_object(json_object_get_value(downloadResult, "*"));
        }

        workflow_free_file_entity(entity);
        entity = nullptr;

        if (resultForFile != nullptr)
        {
            result.ResultCode = json_object_get_number(resultForFile, "resultCode");
            result.ExtendedResultCode = json_object_get_number(resultForFile, "extendedResultCode");
            workflow_set_result_details(handle, json_object_get_string(resultForFile, "resultDetails"));
        }
        else
        {
            result = { .ResultCode = ADUC_Result_Download_Success };
        }

        if (IsAducResultCodeFailure(result.ResultCode))
        {
            goto done;
        }
    }

done:
    STRING_delete(leafManifestFile);
    STRING_delete(childId);

    if (data != nullptr)
    {
        json_value_free(json_object_get_wrapping_value(data));
    }

    return result;
}

ADUC_Result SimulatorActionHelper(
    const ADUC_WorkflowData* workflowData,
    ADUC_Result_t defaultResultCode,
    const char* action,
    const char* resultSelector)
{
    ADUC_Result result = { .ResultCode = defaultResultCode };
    ADUC_WorkflowHandle handle = workflowData->WorkflowHandle;
    ADUC_WorkflowHandle childHandle = nullptr;

    JSON_Object* resultObject = nullptr;

    JSON_Object* data = ReadDataFile();
    if (data == nullptr)
    {
        Log_Info("No simulator data file provided, returning default result code...");
        result = { .ResultCode = defaultResultCode };
        goto done;
    }

    // Get results group from specified 'action'.
    resultObject = json_value_get_object(json_object_get_value(data, action));

    // Select for specific result.
    if (!IsNullOrEmpty(resultSelector))
    {
        JSON_Object* selectResult = json_value_get_object(json_object_get_value(resultObject, resultSelector));

        // Fall back to catch-all result (if specified in the data file).
        if (selectResult == nullptr)
        {
            selectResult = json_value_get_object(json_object_get_value(resultObject, "*"));
        }

        resultObject = selectResult;
    }

    if (resultObject != nullptr)
    {
        result.ResultCode = json_object_get_number(resultObject, "resultCode");
        result.ExtendedResultCode = json_object_get_number(resultObject, "extendedResultCode");

        if (workflowData->WorkflowHandle != nullptr)
        {
            workflow_set_result_details(handle, json_object_get_string(resultObject, "resultDetails"));
        }
    }

    // For 'microsoft/bundle:1' implementation, abort download task as soon as an error occurs.
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

done:
    if (data != nullptr)
    {
        json_value_free(json_object_get_wrapping_value(data));
    }
    return result;
}

/**
 * @brief Mock implementation of install
 * @return ADUC_Result Return result from simulator data file if specified.
 *         Otherwise, return ADUC_Result_Install_Success.
 */
ADUC_Result SimulatorHandlerImpl::Install(const ADUC_WorkflowData* workflowData)
{
    return SimulatorActionHelper(workflowData, ADUC_Result_Install_Success, "install", nullptr);
}

/**
 * @brief Mock implementation of apply
 * @return ADUC_Result Return result from simulator data file if specified.
 *         Otherwise, return ADUC_Result_Apply_Success.
 */
ADUC_Result SimulatorHandlerImpl::Apply(const ADUC_WorkflowData* workflowData)
{
    return SimulatorActionHelper(workflowData, ADUC_Result_Apply_Success, "apply", nullptr);
}

/**
 * @brief Mock implementation of cancel
 * @return ADUC_Result Return result from simulator data file if specified.
 *         Otherwise, return ADUC_Result_Cancel_Success.
 */
ADUC_Result SimulatorHandlerImpl::Cancel(const ADUC_WorkflowData* workflowData)
{
    return SimulatorActionHelper(workflowData, ADUC_Result_Cancel_Success, "cancel", nullptr);
}

/**
 * @brief Mock implementation of IsInstalled check.
 * @return ADUC_Result The result based on evaluating the installed criteria.
 */
ADUC_Result SimulatorHandlerImpl::IsInstalled(const ADUC_WorkflowData* workflowData)
{
    char* installedCriteria = ADUC_WorkflowData_GetInstalledCriteria(workflowData);
    ADUC_Result result =
        SimulatorActionHelper(workflowData, ADUC_Result_IsInstalled_Installed, "isInstalled", installedCriteria);
    workflow_free_string(installedCriteria);
    return result;
}