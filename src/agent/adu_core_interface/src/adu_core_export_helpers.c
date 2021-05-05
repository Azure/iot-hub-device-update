/**
 * @file adu_core_export_helpers.c
 * @brief Provides set of helpers for creating objects defined in adu_core_exports.h
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#include "aduc/adu_core_export_helpers.h"

#include <stdbool.h>
#include <stdlib.h>

#include <parson.h>

#include "aduc/adu_core_interface.h"
#include "aduc/c_utils.h"
#include <aduc/logging.h>
#include <aduc/string_c_utils.h>
#include <sys/wait.h> // for waitpid
#include <unistd.h>

void ADUC_MethodCall_Idle(ADUC_WorkflowData* workflowData);

/**
 * @brief Move state machine to a new stage.
 *
 * @param[in,out] workflowData Workflow data.
 * @param[in] updateState New update state to transition to.
 * @param[in] result Result to report (optional, can be NULL).
 */
static void
ADUC_SetUpdateStateHelper(ADUC_WorkflowData* workflowData, ADUCITF_State updateState, const ADUC_Result* result)
{
    Log_Info("Setting UpdateState to %s", ADUCITF_StateToString(updateState));

    // If we're transitioning from Apply_Started to Idle, we need to report InstalledUpdateId.
    //  if apply succeeded.
    // This is required by ADU service.
    if (updateState == ADUCITF_State_Idle)
    {
        if (workflowData->LastReportedState == ADUCITF_State_ApplyStarted)
        {
            if (workflowData->SystemRebootState == ADUC_SystemRebootState_None
                && workflowData->AgentRestartState == ADUC_AgentRestartState_None)
            {
                // Apply completed, if no reboot or restart is needed, then report deployment succeeded
                // to the ADU service to complete the update workflow.
                ADUC_SetInstalledUpdateIdAndGoToIdle(workflowData, workflowData->ContentData->ExpectedUpdateId);
                workflowData->LastReportedState = updateState;
                return;
            }

            if (workflowData->SystemRebootState == ADUC_SystemRebootState_InProgress)
            {
                // Reboot is required, and successfully initiated (device is shutting down and restarting).
                // We want to transition to an Idle state internally, but will not report the state to the ADU service,
                // since the InstallUpdateId will not be accurate until the device rebooted.
                //
                // Note: if we report Idle state and InstallUpdateId doesn't match ExpectedUpdateId,
                // ADU service will consider the update failed.
                ADUC_MethodCall_Idle(workflowData);
                workflowData->OperationCancelled = false;
                workflowData->OperationInProgress = false;
                return;
            }

            if (workflowData->AgentRestartState == ADUC_AgentRestartState_InProgress)
            {
                // Agent restart is required, and successfully initiated.
                // We want to transition to an Idle state internally, but will not report the state to the ADU service,
                // until the agent restarted.
                //
                // Note: if we report Idle state and InstallUpdateId doesn't match ExpectedUpdateId,
                // ADU service will consider the update failed.
                ADUC_MethodCall_Idle(workflowData);
                workflowData->OperationCancelled = false;
                workflowData->OperationInProgress = false;
                return;
            }

            // Device failed to reboot, or the agent failed to restart, consider update failed.
            // Fall through to report Idle without InstalledUpdateId.
        }

        AzureDeviceUpdateCoreInterface_ReportStateAndResultAsync(updateState, result);
        ADUC_MethodCall_Idle(workflowData);
        workflowData->OperationCancelled = false;
        workflowData->OperationInProgress = false;
    }
    else
    {
        AzureDeviceUpdateCoreInterface_ReportStateAndResultAsync(updateState, result);
    }

    workflowData->LastReportedState = updateState;
}

/**
 * @brief Set a new update state.
 *
 * @param[in,out] workflowData Workflow data object.
 * @param[in] updateState New update state.
 */
void ADUC_SetUpdateState(ADUC_WorkflowData* workflowData, ADUCITF_State updateState)
{
    ADUC_SetUpdateStateHelper(workflowData, updateState, NULL /*result*/);
}

/**
 * @brief Set a new update state and result.
 *
 * @param[in,out] workflowData Workflow data object.
 * @param[in] updateState New update state.
 * @param[in] result Result to report.
 */
void ADUC_SetUpdateStateWithResult(ADUC_WorkflowData* workflowData, ADUCITF_State updateState, ADUC_Result result)
{
    ADUC_SetUpdateStateHelper(workflowData, updateState, &result);
}

/**
 * @brief Sets installedUpdateId to the given update ID and sets state to Idle.
 *
 * @param[in,out] workflowData The workflow data.
 * @param[in] updateId The updateId for the installed content.
 */
void ADUC_SetInstalledUpdateIdAndGoToIdle(ADUC_WorkflowData* workflowData, const ADUC_UpdateId* updateId)
{
    AzureDeviceUpdateCoreInterface_ReportUpdateIdAndIdleAsync(updateId);

    workflowData->LastReportedState = ADUCITF_State_Idle;

    ADUC_MethodCall_Idle(workflowData);

    workflowData->OperationCancelled = false;
    workflowData->OperationInProgress = false;
    workflowData->SystemRebootState = ADUC_SystemRebootState_None;
    workflowData->AgentRestartState = ADUC_AgentRestartState_None;
}

//
// ADUC_PrepareInfo helpers.
//
_Bool ADUC_PrepareInfo_Init(ADUC_PrepareInfo* info, const ADUC_WorkflowData* workflowData)
{
    _Bool succeeded = false;

    // Initialize out parameter.
    memset(info, 0, sizeof(*info));

    if (!ADUC_Json_GetFiles(workflowData->UpdateActionJson, &(info->fileCount), &(info->files)))
    {
        goto done;
    }

    if (mallocAndStrcpy_s(&(info->updateType), workflowData->ContentData->UpdateType) != 0)
    {
        goto done;
    }

    succeeded = true;

done:
    if (!succeeded)
    {
        ADUC_PrepareInfo_UnInit(info);
    }
    return succeeded;
}

/**
 * @brief Free members of ADUC_PrepareInfo object.
 *
 * @param info Object to free.
 */
void ADUC_PrepareInfo_UnInit(ADUC_PrepareInfo* info)
{
    if (info == NULL)
    {
        return;
    }

    free(info->updateType);
    free(info->updateTypeName);
    ADUC_FileEntityArray_Free(info->fileCount, info->files);

    memset(info, 0, sizeof(*info));
}

//
// ADUC_DownloadInfo helpers.
//

/**
 * @brief Initialize a ADUC_DownloadInfo object. Caller must free using ADUC_DownloadInfo_UnInit().
 *
 * @param[in,out] info Object to initialize.
 * @param[in] updateActionJson JSON with update action metadata.
 * @param[in] workFolder Sandbox to use for download, can be NULL.
 * @param[in] progressCallback Callback function for reporting download progress.
 * @return _Bool True on success.
 */
_Bool ADUC_DownloadInfo_Init(
    ADUC_DownloadInfo* info,
    const JSON_Value* updateActionJson,
    const char* workFolder,
    ADUC_DownloadProgressCallback progressCallback)
{
    _Bool succeeded = false;

    // Initialize out parameter.
    memset(info, 0, sizeof(*info));

    //
    // Set WorkFolder, NotifyDownloadProgress (not obtained from JSON)
    //

    if (workFolder != NULL)
    {
        if (mallocAndStrcpy_s(&(info->WorkFolder), workFolder) != 0)
        {
            goto done;
        }
    }

    info->NotifyDownloadProgress = progressCallback;

    if (!ADUC_Json_GetFiles(updateActionJson, &(info->FileCount), &(info->Files)))
    {
        goto done;
    }

    succeeded = true;

done:
    if (!succeeded)
    {
        ADUC_DownloadInfo_UnInit(info);
    }

    return succeeded;
}

/**
 * @brief Free ADUC_FileEntity object.
 *
 * Caller should assume files object is invalid after this method returns.
 *
 * @param fileCount Count of objects in files.
 * @param files Array of ADUC_FileEntity objects to free.
 */
void ADUC_FileEntityArray_Free(unsigned int fileCount, ADUC_FileEntity* files)
{
    for (unsigned int index = 0; index < fileCount; ++index)
    {
        ADUC_FileEntity* entity = files + index;

        free(entity->DownloadUri);
        free(entity->TargetFilename);
        free(entity->FileId);
        ADUC_Hash_FreeArray(entity->HashCount, entity->Hash);
    }
    free(files);
}

/**
 * @brief Frees an array of ADUC_Hashes of size @p hashCount
 * @param hashCount the size of @p hashArray
 * @param hashArray a pointer to an array of ADUC_Hash structs
 */
void ADUC_Hash_FreeArray(size_t hashCount, ADUC_Hash* hashArray)
{
    for (size_t hash_index = 0; hash_index < hashCount; ++hash_index)
    {
        ADUC_Hash* hashEntity = hashArray + hash_index;
        ADUC_Hash_UnInit(hashEntity);
    }
    free(hashArray);
}

void ADUC_Hash_UnInit(ADUC_Hash* hash)
{
    free(hash->value);
    hash->value = NULL;

    free(hash->type);
    hash->type = NULL;
}

_Bool ADUC_Hash_Init(ADUC_Hash* hash, const char* hashValue, const char* hashType)
{
    _Bool success = false;

    if (hash == NULL)
    {
        return false;
    }

    if (hashValue == NULL || hashType == NULL)
    {
        Log_Error("Invalid call to ADUC_Hash_Init with hashValue %s and hashType %s", hashValue, hashType);
        return false;
    }

    hash->value = NULL;
    hash->type = NULL;

    if (mallocAndStrcpy_s(&(hash->value), hashValue) != 0)
    {
        goto done;
    }

    if (mallocAndStrcpy_s(&(hash->type), hashType) != 0)
    {
        goto done;
    }

    success = true;

done:

    if (!success)
    {
        ADUC_Hash_UnInit(hash);
    }

    return success;
}
/**
 * @brief Free ADUC_UpdateId object
 * @details Caller should assume update id object is invalid after this method returns
 *
 * @param updateId the id to be freed
 *
 */
void ADUC_UpdateId_Free(ADUC_UpdateId* updateId)
{
    if (updateId == NULL)
    {
        return;
    }

    free(updateId->Provider);

    free(updateId->Name);

    free(updateId->Version);

    free(updateId);
}

/**
 * @brief Allocates and sets the UpdateId fields
 * @details Caller should free the allocated ADUC_UpdateId* using ADUC_UpdateId_Free()
 * @param provider the provider for the UpdateId
 * @param name the name for the UpdateId
 * @param version the version for the UpdateId
 *
 * @returns An UpdateId on success, NULL on failure
 */
ADUC_UpdateId* ADUC_UpdateId_AllocAndInit(const char* provider, const char* name, const char* version)
{
    _Bool success = false;

    ADUC_UpdateId* updateId = (ADUC_UpdateId*)calloc(1, sizeof(ADUC_UpdateId));

    if (updateId == NULL)
    {
        Log_Error("ADUC_UpdateId_AllocAndInit called with a NULL updateId handle");
        goto done;
    }

    if (provider == NULL || name == NULL || version == NULL)
    {
        Log_Error(
            "Invalid call to ADUC_UpdateId_AllocAndInit with provider %s name %s version %s", provider, name, version);
        goto done;
    }

    if (mallocAndStrcpy_s(&(updateId->Provider), provider) != 0)
    {
        goto done;
    }

    if (mallocAndStrcpy_s(&(updateId->Name), name) != 0)
    {
        goto done;
    }

    if (mallocAndStrcpy_s(&(updateId->Version), version) != 0)
    {
        goto done;
    }

    success = true;

done:

    if (!success)
    {
        ADUC_UpdateId_Free(updateId);
        updateId = NULL;
    }

    return updateId;
}

/**
 * @brief Takes in an updateId and serializes to a string
 * @details Caller is responsible for using the Parson json_free_serialized_string to de-allocate the returned string
 *
 * Schema:
 * {
 *      ...
 *      InstalledUpdateId:"{
 *          "provider":<provider-str>,
 *          "name":<name-str>,
 *          "version":<version-str>
 *      }"
 *      ...
 * }
 * @param updateId an updateId to be transformed into a serialized string
 * @returns NULL on failure and a serialized string on success
 */
char* ADUC_UpdateIdToJsonString(const ADUC_UpdateId* updateId)
{
    if (updateId == NULL)
    {
        return NULL;
    }

    char* updateIdJSONString = NULL;

    JSON_Value* installedUpdateIdValue = json_value_init_object();
    JSON_Object* installedUpdateIdObject = json_value_get_object(installedUpdateIdValue);

    if (installedUpdateIdObject == NULL)
    {
        Log_Error("Could not allocate an JSON_Object for the UpdateId");
        goto done;
    }

    JSON_Status jsonStatus =
        json_object_set_string(installedUpdateIdObject, ADUCITF_FIELDNAME_PROVIDER, updateId->Provider);
    if (jsonStatus != JSONSuccess)
    {
        Log_Error(
            "Could not serialize updateId's JSON field: %s value: %s", ADUCITF_FIELDNAME_PROVIDER, updateId->Provider);
        goto done;
    }

    jsonStatus = json_object_set_string(installedUpdateIdObject, ADUCITF_FIELDNAME_NAME, updateId->Name);
    if (jsonStatus != JSONSuccess)
    {
        Log_Error("Could not serialize updateId's JSON field: %s value: %s", ADUCITF_FIELDNAME_NAME, updateId->Name);
        goto done;
    }

    jsonStatus = json_object_set_string(installedUpdateIdObject, ADUCITF_FIELDNAME_VERSION, updateId->Version);
    if (jsonStatus != JSONSuccess)
    {
        Log_Error(
            "Could not serialize updateId's JSON field: %s value: %s", ADUCITF_FIELDNAME_VERSION, updateId->Version);
        goto done;
    }

    updateIdJSONString = json_serialize_to_string(installedUpdateIdValue);

    if (updateIdJSONString == NULL)
    {
        goto done;
    }
done:

    if (installedUpdateIdValue != NULL)
    {
        json_value_free(installedUpdateIdValue);
    }

    return updateIdJSONString;
}

/**
 * @brief Checks if the UpdateId is valid
 * @param updateId updateId to check
 * @returns True if it is valid, false if not
 */
_Bool ADUC_IsValidUpdateId(const ADUC_UpdateId* updateId)
{
    _Bool success = false;

    if (updateId == NULL || updateId->Provider == NULL || updateId->Name == NULL || updateId->Version == NULL)
    {
        goto done;
    }

    if (*(updateId->Provider) == '\0' || *(updateId->Name) == '\0' || *(updateId->Version) == '\0')
    {
        goto done;
    }

    success = true;

done:
    return success;
}

/**
 * @brief Free members of ADUC_DownloadInfo object.
 *
 * @param info Object to free.
 */
void ADUC_DownloadInfo_UnInit(ADUC_DownloadInfo* info)
{
    if (info == NULL)
    {
        return;
    }

    free(info->WorkFolder);

    ADUC_FileEntityArray_Free(info->FileCount, info->Files);

    memset(info, 0, sizeof(*info));
}

//
// ADUC_InstallInfo helpers.
//

/**
 * @brief Initialize a ADUC_InstallInfo object. Caller must free using ADUC_InstallInfo_UnInit().
 *
 * @param info Object to initialize.
 * @param workFolder Sandbox to use for install, can be NULL.
 * @return _Bool True on success.
 */
_Bool ADUC_InstallInfo_Init(ADUC_InstallInfo* info, const char* workFolder)
{
    _Bool succeeded = false;

    // Initialize out parameter.
    memset(info, 0, sizeof(*info));

    if (workFolder != NULL)
    {
        if (mallocAndStrcpy_s(&(info->WorkFolder), workFolder) != 0)
        {
            goto done;
        }
    }

    succeeded = true;

done:
    if (!succeeded)
    {
        ADUC_InstallInfo_UnInit(info);
    }

    return succeeded;
}

/**
 * @brief Free members of ADUC_InstallInfo object.
 *
 * @param info Object to free.
 */
void ADUC_InstallInfo_UnInit(ADUC_InstallInfo* info)
{
    if (info == NULL)
    {
        return;
    }

    free(info->WorkFolder);

    memset(info, 0, sizeof(*info));
}

//
// ADUC_ApplyInfo helpers.
//

/**
 * @brief Initialize a ADUC_ApplyInfo object. Caller must free using ADUC_ApplyInfo_UnInit().
 *
 * @param info Object to initialize.
 * @param workFolder Sandbox to use for apply, can be NULL.
 * @return _Bool True on success,.
 */
_Bool ADUC_ApplyInfo_Init(ADUC_ApplyInfo* info, const char* workFolder)
{
    _Bool succeeded = false;

    // Initialize out parameter.
    memset(info, 0, sizeof(*info));

    if (workFolder != NULL)
    {
        if (mallocAndStrcpy_s(&(info->WorkFolder), workFolder) != 0)
        {
            goto done;
        }
    }

    succeeded = true;

done:
    if (!succeeded)
    {
        ADUC_ApplyInfo_UnInit(info);
    }

    return succeeded;
}

/**
 * @brief Free members of ADUC_ApplyInfo object.
 *
 * @param info Object to free.
 */
void ADUC_ApplyInfo_UnInit(ADUC_ApplyInfo* info)
{
    if (info == NULL)
    {
        return;
    }

    free(info->WorkFolder);

    memset(info, 0, sizeof(*info));
}

//
// ADUC_RegisterData helpers.
//

/**
 * @brief Check to see if a ADUC_RegisterData object is valid.
 *
 * @param registerData Object to verify.
 * @return _Bool True if valid.
 */
static _Bool ADUC_RegisterData_VerifyData(const ADUC_RegisterData* registerData)
{
    // Note: Okay for registerData->token to be NULL.

    if (registerData->IdleCallback == NULL || registerData->DownloadCallback == NULL
        || registerData->InstallCallback == NULL || registerData->ApplyCallback == NULL
        || registerData->SandboxCreateCallback == NULL || registerData->SandboxDestroyCallback == NULL
        || registerData->PrepareCallback == NULL || registerData->DoWorkCallback == NULL
        || registerData->IsInstalledCallback == NULL)
    {
        Log_Error("Invalid ADUC_RegisterData object");
        return false;
    }

    return true;
}

//
// Register/Unregister methods.
//

/**
 * @brief Call into upper layer ADUC_Register() method.
 *
 * @param registerData Metadata for call.
 * @param argc Count of arguments in @p argv
 * @param argv Arguments to pass to ADUC_Register().
 * @return ADUC_Result Result code.
 */
ADUC_Result ADUC_MethodCall_Register(ADUC_RegisterData* registerData, unsigned int argc, const char** argv)
{
    Log_Info("Calling ADUC_Register");

    ADUC_Result result = ADUC_Register(registerData, argc, argv);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        return result;
    }

    if (!ADUC_RegisterData_VerifyData(registerData))
    {
        Log_Error("Invalid ADUC_RegisterData structure");

        result.ResultCode = ADUC_RegisterResult_Failure;
        result.ExtendedResultCode = ADUC_ERC_NOTRECOVERABLE;
        return result;
    }

    result.ResultCode = ADUC_RegisterResult_Success;
    return result;
}

/**
 * @brief Call into upper layer ADUC_Unregister() method.
 *
 * @param registerData Metadata for call.
 */
void ADUC_MethodCall_Unregister(const ADUC_RegisterData* registerData)
{
    Log_Info("Calling ADUC_Unregister");

    ADUC_Unregister(registerData->Token);
}

/**
 * @brief Call into upper layer ADUC_RebootSystem() method.
 *
 * @param workflowData Metadata for call.
 * 
 * @returns int errno, 0 if success.
 */
int ADUC_MethodCall_RebootSystem()
{
    Log_Info("Calling ADUC_RebootSystem");

    return ADUC_RebootSystem();
}

/**
 * @brief Call into upper layer ADUC_RestartAgent() method.
 *
 * @param workflowData Metadata for call.
 * 
 * @returns int errno, 0 if success.
 */
int ADUC_MethodCall_RestartAgent()
{
    Log_Info("Calling ADUC_RestartAgent");

    return ADUC_RestartAgent();
}

//
// ADU Core Interface methods.
//

/**
 * @brief Called when entering Idle state.
 *
 * Idle state is the "ready for new workflow state"
 *
 * @param workflowData Workflow metadata.
 */
void ADUC_MethodCall_Idle(ADUC_WorkflowData* workflowData)
{
    const ADUC_RegisterData* registerData = &(workflowData->RegisterData);

    //
    // Clean up the sandbox.  It will be re-created when download starts.
    //

    if (workflowData->WorkFolder != NULL)
    {
        Log_Info("Calling SandboxDestroyCallback");

        registerData->SandboxDestroyCallback(registerData->Token, workflowData->WorkflowId, workflowData->WorkFolder);

        free(workflowData->WorkFolder);
        workflowData->WorkFolder = NULL;

        Log_Info("UpdateAction: Idle. Ending workflow with WorkflowId: %s", workflowData->WorkflowId);
        workflowData->WorkflowId[0] = 0;
    }

    else
    {
        Log_Info("UpdateAction: Idle. WorkflowId is not generated yet.");
    }

    // Can reach Idle state from ApplyStarted as there isn't an ApplySucceeded state.
    if (workflowData->LastReportedState != ADUCITF_State_Idle
        && workflowData->LastReportedState != ADUCITF_State_ApplyStarted
        && workflowData->LastReportedState != ADUCITF_State_Failed)
    {
        // Likely nothing we can do about this, but try setting Idle state again.
        Log_Warn(
            "Idle UpdateAction called in unexpected state: %s!",
            ADUCITF_StateToString(workflowData->LastReportedState));
    }

    //
    // Notify callback that we're now back to idle.
    //

    Log_Info("Calling IdleCallback");

    registerData->IdleCallback(registerData->Token, workflowData->WorkflowId);
}

/**
 * @brief Called to request platform-layer operation to prepare.
 * @param[in,out] methodCallData The metedata for the method call.
 * @return Result code.
 */
ADUC_Result ADUC_MethodCall_Prepare(const ADUC_WorkflowData* workflowData)
{
    Log_Info("UpdateAction: Prepare - calling PrepareCallback");

    const ADUC_RegisterData* registerData = &(workflowData->RegisterData);
    ADUC_Result result = { ADUC_PrepareResult_Failure };
    ADUC_PrepareInfo info = {};

    if (!ADUC_PrepareInfo_Init(&info, workflowData))
    {
        result.ResultCode = ADUC_PrepareResult_Failure;
        result.ExtendedResultCode = ADUC_ERC_NOTRECOVERABLE;
        goto done;
    }

    // split updateType string to get updateType name and version
    if (!ADUC_ParseUpdateType(info.updateType, &(info.updateTypeName), &(info.updateTypeVersion)))
    {
        Log_Error("Unsupported update type: '%s'", info.updateType);
        goto done;
    }

    result = registerData->PrepareCallback(registerData->Token, workflowData->WorkflowId, &info);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("Failed to prepare for update, code: %d", result.ResultCode);
        goto done;
    }

done:
    ADUC_PrepareInfo_UnInit(&info);
    return result;
}

/**
 * @brief Called to do download.
 *
 * @param[in,out] methodCallData The metedata for the method call.
 * @return Result code.
 */
ADUC_Result ADUC_MethodCall_Download(ADUC_MethodCall_Data* methodCallData)
{
    ADUC_WorkflowData* workflowData = methodCallData->WorkflowData;
    const ADUC_RegisterData* registerData = &(workflowData->RegisterData);
    ADUC_Result result = { ADUC_DownloadResult_Success };
    ADUC_DownloadInfo* info = NULL;

    Log_Info("UpdateAction: Download");

    methodCallData->MethodSpecificData.DownloadInfo = NULL;

    if (workflowData->LastReportedState != ADUCITF_State_Idle)
    {
        Log_Error(
            "Download UpdateAction called in unexpected state: %s!",
            ADUCITF_StateToString(workflowData->LastReportedState));
        result.ResultCode = ADUC_DownloadResult_Failure;
        result.ExtendedResultCode = ADUC_ERC_NOTPERMITTED;
        goto done;
    }

    free(workflowData->WorkFolder);
    workflowData->WorkFolder = NULL;

    Log_Info("Calling SandboxCreateCallback");

    // Note: It's okay for SandboxCreate to return NULL for the work folder.
    // NULL likely indicates an OS without a file system.
    result = registerData->SandboxCreateCallback(
        registerData->Token, workflowData->WorkflowId, &(workflowData->WorkFolder));
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    Log_Info("Using sandbox %s", workflowData->WorkFolder);

    ADUC_SetUpdateState(workflowData, ADUCITF_State_DownloadStarted);

    info = calloc(1, sizeof(ADUC_DownloadInfo));
    if (info == NULL)
    {
        result.ResultCode = ADUC_DownloadResult_Failure;
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    if (!ADUC_DownloadInfo_Init(
            info, workflowData->UpdateActionJson, workflowData->WorkFolder, workflowData->DownloadProgressCallback))
    {
        result.ResultCode = ADUC_DownloadResult_Failure;
        result.ExtendedResultCode = ADUC_ERC_NOTRECOVERABLE;
        goto done;
    }

    // methodSpecificData owns info now - will be freed when method completes.
    methodCallData->MethodSpecificData.DownloadInfo = info;
    info = NULL;

    Log_Info("Calling DownloadCallback");

    result = registerData->DownloadCallback(
        registerData->Token,
        workflowData->WorkflowId,
        workflowData->ContentData->UpdateType,
        &(methodCallData->WorkCompletionData),
        methodCallData->MethodSpecificData.DownloadInfo);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

done:
    ADUC_DownloadInfo_UnInit(info);
    return result;
}

void ADUC_MethodCall_Download_Complete(ADUC_MethodCall_Data* methodCallData, ADUC_Result result)
{
    UNREFERENCED_PARAMETER(result);

    ADUC_DownloadInfo* info = methodCallData->MethodSpecificData.DownloadInfo;

    ADUC_DownloadInfo_UnInit(info);
    methodCallData->MethodSpecificData.DownloadInfo = NULL;
}

/**
 * @brief Called to do install.
 *
 * @param workflowData Workflow metadata.
 * @return Result code.
 */
ADUC_Result ADUC_MethodCall_Install(ADUC_MethodCall_Data* methodCallData)
{
    ADUC_WorkflowData* workflowData = methodCallData->WorkflowData;
    const ADUC_RegisterData* registerData = &(workflowData->RegisterData);
    ADUC_Result result;
    ADUC_InstallInfo* info = NULL;

    Log_Info("UpdateAction: Install");

    methodCallData->MethodSpecificData.InstallInfo = NULL;

    if (workflowData->LastReportedState != ADUCITF_State_DownloadSucceeded)
    {
        Log_Error(
            "Install UpdateAction called in unexpected state: %s!",
            ADUCITF_StateToString(workflowData->LastReportedState));
        result.ResultCode = ADUC_InstallResult_Failure;
        result.ExtendedResultCode = ADUC_ERC_NOTPERMITTED;
        goto done;
    }

    ADUC_SetUpdateState(workflowData, ADUCITF_State_InstallStarted);

    info = calloc(1, sizeof(ADUC_InstallInfo));
    if (info == NULL)
    {
        result.ResultCode = ADUC_InstallResult_Failure;
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    if (!ADUC_InstallInfo_Init(info, workflowData->WorkFolder))
    {
        result.ResultCode = ADUC_InstallResult_Failure;
        result.ExtendedResultCode = ADUC_ERC_NOTRECOVERABLE;
        goto done;
    }

    // methodSpecificData owns info now - will be freed when method completes.
    methodCallData->MethodSpecificData.InstallInfo = info;
    info = NULL;

    Log_Info("Calling InstallCallback");

    result = registerData->InstallCallback(
        registerData->Token,
        workflowData->WorkflowId,
        &(methodCallData->WorkCompletionData),
        methodCallData->MethodSpecificData.InstallInfo);

done:
    ADUC_InstallInfo_UnInit(info);
    return result;
}

void ADUC_MethodCall_Install_Complete(ADUC_MethodCall_Data* methodCallData, ADUC_Result result)
{
    UNREFERENCED_PARAMETER(result);

    ADUC_InstallInfo* info = methodCallData->MethodSpecificData.InstallInfo;

    ADUC_InstallInfo_UnInit(info);
    methodCallData->MethodSpecificData.InstallInfo = NULL;
}

/**
 * @brief Called to do apply.
 *
 * @param workflowData Workflow metadata.
 * @return Result code.
 */
ADUC_Result ADUC_MethodCall_Apply(ADUC_MethodCall_Data* methodCallData)
{
    ADUC_WorkflowData* workflowData = methodCallData->WorkflowData;
    const ADUC_RegisterData* registerData = &(workflowData->RegisterData);
    ADUC_Result result;
    ADUC_ApplyInfo* info = NULL;

    Log_Info("UpdateAction: Apply");

    methodCallData->MethodSpecificData.ApplyInfo = NULL;

    if (workflowData->LastReportedState != ADUCITF_State_InstallSucceeded)
    {
        Log_Error(
            "Apply UpdateAction called in unexpected state: %s!",
            ADUCITF_StateToString(workflowData->LastReportedState));
        result.ResultCode = ADUC_ApplyResult_Failure;
        result.ExtendedResultCode = ADUC_ERC_NOTPERMITTED;
        goto done;
    }

    ADUC_SetUpdateState(workflowData, ADUCITF_State_ApplyStarted);

    info = calloc(1, sizeof(ADUC_ApplyInfo));
    if (info == NULL)
    {
        result.ResultCode = ADUC_ApplyResult_Failure;
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    if (!ADUC_ApplyInfo_Init(info, workflowData->WorkFolder))
    {
        result.ResultCode = ADUC_ApplyResult_Failure;
        result.ExtendedResultCode = ADUC_ERC_NOTRECOVERABLE;
        goto done;
    }

    // methodSpecificData owns info now - will be freed when method completes.
    methodCallData->MethodSpecificData.ApplyInfo = info;
    info = NULL;

    Log_Info("Calling ApplyCallback");

    result = registerData->ApplyCallback(
        registerData->Token,
        workflowData->WorkflowId,
        &(methodCallData->WorkCompletionData),
        methodCallData->MethodSpecificData.ApplyInfo);

done:
    ADUC_ApplyInfo_UnInit(info);
    return result;
}

void ADUC_MethodCall_Apply_Complete(ADUC_MethodCall_Data* methodCallData, ADUC_Result result)
{
    ADUC_ApplyInfo* info = methodCallData->MethodSpecificData.ApplyInfo;

    ADUC_ApplyInfo_UnInit(info);
    methodCallData->MethodSpecificData.ApplyInfo = NULL;

    if (result.ResultCode == ADUC_ApplyResult_SuccessRebootRequired)
    {
        // If apply indicated a reboot required result from apply, go ahead and reboot.
        Log_Info("Apply indicated success with RebootRequired - rebooting system now");
        methodCallData->WorkflowData->SystemRebootState = ADUC_SystemRebootState_Required;
        int success = ADUC_MethodCall_RebootSystem();
        if (success == 0)
        {
            methodCallData->WorkflowData->SystemRebootState = ADUC_SystemRebootState_InProgress;
        }
        else
        {
            Log_Error("Reboot attempt failed.");
            methodCallData->WorkflowData->OperationInProgress = false;
        }
    }
    else if (result.ResultCode == ADUC_ApplyResult_SuccessAgentRestartRequired)
    {
        // If apply indicated a restart is required, go ahead and restart the agent.
        Log_Info("Apply indicated success with AgentRestartRequired - restarting the agent now");
        methodCallData->WorkflowData->SystemRebootState = ADUC_SystemRebootState_Required;
        int success = ADUC_MethodCall_RestartAgent();
        if (success == 0)
        {
            methodCallData->WorkflowData->AgentRestartState = ADUC_AgentRestartState_InProgress;
        }
        else
        {
            Log_Error("Agent restart attempt failed.");
            methodCallData->WorkflowData->OperationInProgress = false;
        }
    }
    else if (result.ResultCode == ADUC_ApplyResult_Success)
    {
        // An Apply action completed successfully. Continue to the next step.
        methodCallData->WorkflowData->OperationInProgress = false;
    }
}

/**
 * @brief Called to request platform-layer operation to cancel.
 *
 * This method should only be called while another MethodCall is currently active.
 *
 * @param[in] workflowData Workflow metadata.
 */
void ADUC_MethodCall_Cancel(const ADUC_WorkflowData* workflowData)
{
    const ADUC_RegisterData* registerData = &(workflowData->RegisterData);

    Log_Info("UpdateAction: Cancel - calling CancelCallback");

    if (!workflowData->OperationInProgress)
    {
        Log_Warn("Cancel requested without operation in progress - ignoring.");
        return;
    }

    registerData->CancelCallback(registerData->Token, workflowData->WorkflowId);
}

/**
 * @brief Helper to call into the platform layer for IsInstalled.
 *
 * @param[in] workflowData The workflow data.
 *
 * @return ADUC_Result The result of the IsInstalled call.
 */
ADUC_Result ADUC_MethodCall_IsInstalled(const ADUC_WorkflowData* workflowData)
{
    if (workflowData->ContentData == NULL)
    {
        Log_Info("IsInstalled called before installedCriteria has been initialized.");
        ADUC_Result result = { .ResultCode = ADUC_IsInstalledResult_NotInstalled };
        return result;
    }

    const ADUC_RegisterData* registerData = &(workflowData->RegisterData);
    Log_Info("Calling IsInstalledCallback to check if content is installed.");
    return registerData->IsInstalledCallback(
        registerData->Token,
        workflowData->WorkflowId,
        workflowData->ContentData->UpdateType,
        workflowData->ContentData->InstalledCriteria);
}
