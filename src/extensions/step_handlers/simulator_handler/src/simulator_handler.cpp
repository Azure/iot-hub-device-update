/**
 * @file simulator_handler.cpp
 * @brief Implementation of ContentHandler API for update content simulator.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/simulator_handler.hpp"
#include "aduc/logging.h"
#include "aduc/parser_utils.h" // ADUC_FileEntity_Uninit
#include "aduc/workflow_utils.h"
#include <stdarg.h> // for va_*
#include <stdlib.h> // for getenv
#include <string>

#define SIMULATOR_DATA_FILE "du-simulator-data.json"

EXTERN_C_BEGIN

/////////////////////////////////////////////////////////////////////////////
// BEGIN Shared Library Export Functions
//
// These are the function symbols that the device update agent will
// lookup and call.
//

/**
 * @brief Instantiates an Simulator Update Content Handler
 * @return ContentHandler* The created instance.
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

/**
 * @brief Gets the extension contract info.
 *
 * @param[out] contractInfo The extension contract info.
 * @return ADUC_Result The result.
 */
ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo)
{
    contractInfo->majorVer = ADUC_V1_CONTRACT_MAJOR_VER;
    contractInfo->minorVer = ADUC_V1_CONTRACT_MINOR_VER;
    return ADUC_Result{ ADUC_GeneralResult_Success, 0 };
}

//
// END Shared Library Export Functions
/////////////////////////////////////////////////////////////////////////////

/**
 * @brief Retrieve system temporary path with a subfolder.
 *
 * This only returns a folder name, which is neither created nor checked for existence.
 *
 * Loosely based on Boost's implementation, which is:
 * TMPDIR > TMP > TEMP > TEMPDIR > (android only) "/data/local/tmp" > "/tmp"
 *
 * @return const char* Returns the path to the temporary directory. This is a long-lived string.
 */
const char* _GetTemporaryPathName()
{
    const char* env;
    env = getenv("TMPDIR");
    if (env == nullptr)
    {
        env = getenv("TMP");
        if (env == nullptr)
        {
            env = getenv("TEMP");
            if (env == nullptr)
            {
                env = getenv("TEMPDIR");
                if (env == nullptr)
                {
                    static const char* root_tmp_folder = "/tmp";
                    env = root_tmp_folder;
                }
            }
        }
    }

    return env;
}

/**
 * @brief Maximum length for the output string of ADUC_StringFormat()
 */
#define ADUC_STRING_FORMAT_MAX_LENGTH 512

/**
 * @brief Returns string created by formatting a variable number of string arguments with @p fmt
 * @details Caller must free returned string, any formatted string above 512 characters in length will result in a failed call
 * @param fmt format string to be used on parameters
 * @returns in case of string > 512 characters or other failure nullptr, otherwise a pointer to a null-terminated string
 */
char* _StringFormat(const char* fmt, ...)
{
    if (fmt == nullptr)
    {
        return nullptr;
    }

    char buffer[ADUC_STRING_FORMAT_MAX_LENGTH];

    va_list args;

    va_start(args, fmt);

    const int result = vsnprintf(buffer, ADUC_STRING_FORMAT_MAX_LENGTH, fmt, args);

    va_end(args);

    if (result <= 0 || result >= ADUC_STRING_FORMAT_MAX_LENGTH)
    {
        return nullptr;
    }

    auto outputStr = static_cast<char*>(malloc(strlen(buffer) + 1));
    if (outputStr == nullptr)
    {
        return nullptr;
    }

    if (strcpy(outputStr, buffer) == nullptr)
    {
        free(outputStr);
        outputStr = nullptr;
    }

    return outputStr;
}

EXTERN_C_END

/**
 * @brief Destructor for the Simulator Handler Impl class.
 */
SimulatorHandlerImpl::~SimulatorHandlerImpl() // override
{
    ADUC_Logging_Uninit();
}

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
    return _StringFormat("%s/%s", _GetTemporaryPathName(), SIMULATOR_DATA_FILE);
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
ADUC_Result SimulatorHandlerImpl::Download(const tagADUC_WorkflowData* workflowData)
{
    ADUC_Result result = { .ResultCode = ADUC_Result_Download_Success };
    ADUC_WorkflowHandle handle = workflowData->WorkflowHandle;
    ADUC_WorkflowHandle childHandle = nullptr;
    ADUC_FileEntity fileEntity;
    memset(&fileEntity, 0, sizeof(fileEntity));

    auto fileCount = static_cast<unsigned int>(workflow_get_update_files_count(handle));

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
        result = { .ResultCode = ADUC_Result_Download_Success };

        if (!workflow_get_update_file(handle, i, &fileEntity))
        {
            result = { .ResultCode = ADUC_Result_Failure,
                       .ExtendedResultCode = ADUC_ERC_STEPS_HANDLER_GET_FILE_ENTITY_FAILURE };
            goto done;
        }

        Log_Info("Downloading file#%d (targetFileName:%s).", i, fileEntity.TargetFilename);

        JSON_Object* resultForFile =
            json_value_get_object(json_object_get_value(downloadResult, fileEntity.TargetFilename));

        if (resultForFile == nullptr)
        {
            Log_Info("No matching results for file '%s', fallback to catch-all result", fileEntity.TargetFilename);

            resultForFile = json_value_get_object(json_object_get_value(downloadResult, "*"));
        }

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
    ADUC_FileEntity_Uninit(&fileEntity);

    if (data != nullptr)
    {
        json_value_free(json_object_get_wrapping_value(data));
    }

    return result;
}

ADUC_Result SimulatorActionHelper(
    const tagADUC_WorkflowData* workflowData,
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
    if (resultSelector != nullptr && *resultSelector != '\0')
    {
        JSON_Object* selectResult = json_value_get_object(json_object_get_value(resultObject, resultSelector));

        // Fall back to catch-all result (if specified in the data file).
        if (selectResult == nullptr)
        {
            selectResult = json_value_get_object(json_object_get_value(resultObject, "*"));
        }

        resultObject = selectResult;
    }
    else if (0 == strcmp(action, "isInstalled"))
    {
        // For update manifest version 4, top-level (parent) update doesn't have installed criteria property.
        // In this case, we'll return the catch-all (*) result instead.
        resultObject = json_value_get_object(json_object_get_value(resultObject, "*"));
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
 * @brief Mock implementation of backup
 * @return ADUC_Result Return result from simulator data file if specified.
 *         Otherwise, return ADUC_Result_Backup_Success.
 */
ADUC_Result SimulatorHandlerImpl::Backup(const tagADUC_WorkflowData* workflowData)
{
    return SimulatorActionHelper(workflowData, ADUC_Result_Backup_Success, "backup", nullptr);
}

/**
 * @brief Mock implementation of install
 * @return ADUC_Result Return result from simulator data file if specified.
 *         Otherwise, return ADUC_Result_Install_Success.
 */
ADUC_Result SimulatorHandlerImpl::Install(const tagADUC_WorkflowData* workflowData)
{
    return SimulatorActionHelper(workflowData, ADUC_Result_Install_Success, "install", nullptr);
}

/**
 * @brief Mock implementation of restore
 * @return ADUC_Result Return result from simulator data file if specified.
 *         Otherwise, return ADUC_Result_Restore_Success.
 */
ADUC_Result SimulatorHandlerImpl::Restore(const tagADUC_WorkflowData* workflowData)
{
    return SimulatorActionHelper(workflowData, ADUC_Result_Restore_Success, "restore", nullptr);
}

/**
 * @brief Mock implementation of apply
 * @return ADUC_Result Return result from simulator data file if specified.
 *         Otherwise, return ADUC_Result_Apply_Success.
 */
ADUC_Result SimulatorHandlerImpl::Apply(const tagADUC_WorkflowData* workflowData)
{
    return SimulatorActionHelper(workflowData, ADUC_Result_Apply_Success, "apply", nullptr);
}

/**
 * @brief Mock implementation of cancel
 * @return ADUC_Result Return result from simulator data file if specified.
 *         Otherwise, return ADUC_Result_Cancel_Success.
 */
ADUC_Result SimulatorHandlerImpl::Cancel(const tagADUC_WorkflowData* workflowData)
{
    return SimulatorActionHelper(workflowData, ADUC_Result_Cancel_Success, "cancel", nullptr);
}

/**
 * @brief Mock implementation of IsInstalled check.
 * @return ADUC_Result The result based on evaluating the installed criteria.
 */
ADUC_Result SimulatorHandlerImpl::IsInstalled(const tagADUC_WorkflowData* workflowData)
{
    char* installedCriteria = workflow_get_installed_criteria(workflowData->WorkflowHandle);

    ADUC_Result result =
        SimulatorActionHelper(workflowData, ADUC_Result_IsInstalled_Installed, "isInstalled", installedCriteria);
    workflow_free_string(installedCriteria);
    return result;
}
