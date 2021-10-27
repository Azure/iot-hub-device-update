/**
 * @file linux_adu_core_impl.cpp
 * @brief Implements an ADUC reference implementation library.
 *
 * @copyright Copyright (c) 2019, Microsoft Corporation.
 */
#include "linux_adu_core_impl.hpp"
#include "aduc/content_handler.hpp"
#include "aduc/content_handler_factory.hpp"
#include "aduc/hash_utils.h"
#include "aduc/string_c_utils.h"
#include "aduc/string_utils.hpp"
#include "aduc/system_utils.h"
#include "aduc/types/workflow.h"
#include "aduc/workflow_data_utils.h"
#include "aduc/workflow_utils.h"

#include <cstring>
#include <grp.h> // for getgrnam
#include <pwd.h> // for getpwnam
#include <sys/stat.h>
#include <unistd.h>

#include <memory>
#include <sstream>
#include <system_error>
#include <vector>

using ADUC::LinuxPlatformLayer;

/**
 * @brief Factory method for LinuxPlatformLayer
 * @return std::unique_ptr<LinuxPlatformLayer> The newly created LinuxPlatformLayer object.
 */
std::unique_ptr<LinuxPlatformLayer> LinuxPlatformLayer::Create()
{
    return std::unique_ptr<LinuxPlatformLayer>{ new LinuxPlatformLayer() };
}

/**
 * @brief Set the ADUC_UpdateActionCallbacks object
 *
 * @param data Object to set.
 * @return ADUC_Result ResultCode.
 */
ADUC_Result LinuxPlatformLayer::SetUpdateActionCallbacks(ADUC_UpdateActionCallbacks* data)
{
    // Message handlers.
    data->IdleCallback = IdleCallback;
    data->DownloadCallback = DownloadCallback;
    data->InstallCallback = InstallCallback;
    data->ApplyCallback = ApplyCallback;
    data->CancelCallback = CancelCallback;

    data->IsInstalledCallback = IsInstalledCallback;

    data->SandboxCreateCallback = SandboxCreateCallback;
    data->SandboxDestroyCallback = SandboxDestroyCallback;

    data->DoWorkCallback = DoWorkCallback;

    // Opaque token, passed to callbacks.

    data->PlatformLayerHandle = this;

    return ADUC_Result{ ADUC_Result_Register_Success };
}

/**
 * @brief Class implementation of Idle method.
 */
void LinuxPlatformLayer::Idle(const char* workflowId)
{
    Log_Info("Now idle. workflowId: %s", workflowId);
    _IsCancellationRequested = false;
}

static ContentHandler* GetContentTypeHandler(const ADUC_WorkflowData* workflowData, char* updateType, ADUC_Result* result)
{
    ContentHandler* contentHandler = nullptr;

#ifdef ADUC_BUILD_UNIT_TESTS
    if (workflowData->TestOverrides != nullptr && workflowData->TestOverrides->ContentHandler_TestOverride != nullptr)
    {
        contentHandler = static_cast<ContentHandler*>(workflowData->TestOverrides->ContentHandler_TestOverride);
    }
    else
#endif
    {
        ADUC_Result loadResult = ContentHandlerFactory::LoadUpdateContentHandlerExtension(updateType, &contentHandler);
        if (IsAducResultCodeFailure(loadResult.ResultCode))
        {
            contentHandler = nullptr;
            *result = loadResult;
        }
    }

    return contentHandler;
}

/**
 * @brief Class implementation of Download method.
 * @return ADUC_Result
 */
ADUC_Result LinuxPlatformLayer::Download(const ADUC_WorkflowData* workflowData)
{
    ADUC_Result result{ ADUC_Result_Failure };

    char* updateType = workflow_get_update_type(workflowData->WorkflowHandle);
    ContentHandler* contentHandler = GetContentTypeHandler(workflowData, updateType, &result);
    if (contentHandler == nullptr)
    {
        goto done;
    }

    result = contentHandler->Download(workflowData);
    if (_IsCancellationRequested)
    {
        result = ADUC_Result{ ADUC_Result_Failure_Cancelled };
        _IsCancellationRequested = false; // For replacement, we can't call Idle so reset here
    }

done:
    workflow_free_string(updateType);

    return result;
}

/**
 * @brief Class implementation of Install method.
 * @return ADUC_Result
 */
ADUC_Result LinuxPlatformLayer::Install(const ADUC_WorkflowData* workflowData)
{
    ADUC_Result result{ ADUC_Result_Failure };

    char* workflowId = workflow_get_id(workflowData->WorkflowHandle);
    char* updateType = workflow_get_update_type(workflowData->WorkflowHandle);

    ContentHandler* contentHandler = GetContentTypeHandler(workflowData, updateType, &result);
    if (contentHandler == nullptr)
    {
        goto done;
    }

    result = contentHandler->Install(workflowData);
    if (_IsCancellationRequested)
    {
        result = ADUC_Result{ ADUC_Result_Failure_Cancelled };
        _IsCancellationRequested = false; // For replacement, we can't call Idle so reset here
    }

done:
    workflow_free_string(workflowId);
    workflow_free_string(updateType);

    return result;
}

/**
 * @brief Class implementation of Apply method.
 * @return ADUC_Result
 */
ADUC_Result LinuxPlatformLayer::Apply(const ADUC_WorkflowData* workflowData)
{
    ADUC_Result result{ ADUC_Result_Failure };

    char* workflowId = workflow_get_id(workflowData->WorkflowHandle);
    char* updateType = workflow_get_update_type(workflowData->WorkflowHandle);

    ContentHandler* contentHandler = GetContentTypeHandler(workflowData, updateType, &result);
    if (contentHandler == nullptr)
    {
        goto done;
    }

    result = contentHandler->Apply(workflowData);
    if (_IsCancellationRequested)
    {
        result = ADUC_Result{ ADUC_Result_Failure_Cancelled };
        _IsCancellationRequested = false; // For replacement, we can't call Idle so reset here
    }

done:
    workflow_free_string(workflowId);
    workflow_free_string(updateType);
    return result;
}

/**
 * @brief Class implementation of Cancel method.
 */
void LinuxPlatformLayer::Cancel(const ADUC_WorkflowData* workflowData)
{
    ADUC_Result result{ ADUC_Result_Failure };
    char* workflowId = workflow_get_id(workflowData->WorkflowHandle);
    char* updateType = workflow_get_update_type(workflowData->WorkflowHandle);

    Log_Info("Cancelling. workflowId: %s", workflowId);

    _IsCancellationRequested = true;
    workflow_free_string(workflowId);

    ContentHandler* contentHandler = GetContentTypeHandler(workflowData, updateType, &result);
    if (contentHandler == nullptr)
    {
        Log_Error("Could not get content handler!");
        goto done;
    }

    // Since this is coming in from main thread, let the content handler know that a cancel has been requested
    // so that it can interrupt the current operation (download, install, apply) that's occurring on the
    // worker thread. Cancel on the contentHandler is blocking call and once content handler confirms the
    // operation has been cancelled, it returns success or failure for the cancel.
    // After each blocking Download, Install, Apply calls above into content handler, it checks if
    // _IsCancellationRequested is true and sets result to ADUC_Result_Failure_Cancelled
    result = contentHandler->Cancel(workflowData);
    if (IsAducResultCodeSuccess(result.ResultCode))
    {
        Log_Info("content handler successfully canceled ongoing operation for workflowId: %s", workflowId);
    }
    else
    {
        Log_Warn("content handler failed to cancel ongoing operation for workflowId: %s", workflowId);
    }

done:
    return;
}

/**
 * @brief Class implementation of the IsInstalled callback.
 * Calls into the content handler to determine if the given installed criteria is installed.
 *
 * @param workflowId The workflow ID.
 * @param installedCriteria The installed criteria for the content.
 * @return ADUC_Result The result of the IsInstalled call.
 */
ADUC_Result LinuxPlatformLayer::IsInstalled(const ADUC_WorkflowData* workflowData)
{
    ContentHandler* contentHandler = nullptr;
    ADUC_Result result;
    char* workflowId = nullptr;
    char* updateType = nullptr;
    char* installedCriteria = nullptr;

    if (workflowData == nullptr)
    {
        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_UPDATE_CONTENT_HANDLER_ISINSTALLED_FAILURE_BAD_UPDATETYPE };
        goto done;
    }

    workflowId = ADUC_WorkflowData_GetWorkflowId(workflowData);
    updateType = ADUC_WorkflowData_GetUpdateType(workflowData);
    installedCriteria = ADUC_WorkflowData_GetInstalledCriteria(workflowData);

    Log_Info("IsInstalled called workflowId: %s, updateType: %s, installed criteria: %s", workflowId, updateType, installedCriteria);

    if (IsNullOrEmpty(updateType))
    {
        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_UPDATE_CONTENT_HANDLER_ISINSTALLED_FAILURE_BAD_UPDATETYPE };
        goto done;
    }

    contentHandler = GetContentTypeHandler(workflowData, updateType, &result);
    if (contentHandler == nullptr)
    {
        goto done;
    }

    result = contentHandler->IsInstalled(workflowData);

done:
    workflow_free_string(workflowId);
    workflow_free_string(updateType);
    workflow_free_string(installedCriteria);
    return result;
}

ADUC_Result LinuxPlatformLayer::SandboxCreate(const char* workflowId, char* workFolder)
{
    struct passwd* pwd = nullptr;
    struct group* grp = nullptr;

    if (IsNullOrEmpty(workflowId))
    {
        Log_Error("Invalid workflowId passed to SandboxCreate! Uninitialized workflow?");
        return ADUC_Result{ ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };
    }

    // Try to delete existing directory.
    int dir_result;
    struct stat sb
    {
    };
    if (stat(workFolder, &sb) == 0 && S_ISDIR(sb.st_mode))
    {
        dir_result = ADUC_SystemUtils_RmDirRecursive(workFolder);
        if (dir_result != 0)
        {
            // Not critical if failed.
            Log_Info("Unable to remove folder %s, error %d", workFolder, dir_result);
        }
    }

    // Note: the return value may point to a static area,
    // and may be overwritten by subsequent calls to getpwent(3), getpwnam(), or getpwuid().
    // (Do not pass the returned pointer to free(3).)
    pwd = getpwnam(ADUC_FILE_USER);
    if (pwd == nullptr)
    {
        return ADUC_Result{ ADUC_Result_Failure, ADUC_ERC_LOWERLEVEL_SANDBOX_CREATE_FAILURE_NO_ADU_USER };
    }

    uid_t aduUserId = pwd->pw_uid;
    pwd = nullptr;

    // Note: The return value may point to a static area,
    // and may be overwritten by subsequent calls to getgrent(3), getgrgid(), or getgrnam().
    // (Do not pass the returned pointer to free(3).)
    grp = getgrnam(ADUC_FILE_GROUP);
    if (grp == nullptr)
    {
        return ADUC_Result{ ADUC_Result_Failure, ADUC_ERC_LOWERLEVEL_SANDBOX_CREATE_FAILURE_NO_ADU_GROUP };
    }
    gid_t aduGroupId = grp->gr_gid;
    grp = nullptr;

    // Create the sandbox folder with adu:adu ownership.
    // Permissions are set to u=rwx,g=rwx. We grant read/write/execute to group owner so that partner
    // processes like the DO daemon can download files to our sandbox.
    dir_result =
        ADUC_SystemUtils_MkDirRecursive(workFolder, aduUserId, aduGroupId, S_IRWXU | S_IRGRP | S_IWGRP | S_IXGRP);
    if (dir_result != 0)
    {
        Log_Error("Unable to create folder %s, error %d", workFolder, dir_result);
        return ADUC_Result{ ADUC_Result_Failure, dir_result };
    }

    Log_Info("Setting sandbox %s", workFolder);

    return ADUC_Result{ ADUC_Result_SandboxCreate_Success };
}

void LinuxPlatformLayer::SandboxDestroy(const char* workflowId, const char* workFolder)
{
    // If SandboxCreate failed or didn't specify a workfolder, we'll get nullptr here.
    if (workFolder == nullptr)
    {
        return;
    }

    Log_Info("Destroying sandbox %s. workflowId: %s", workFolder, workflowId);

    struct stat st = {};
    bool statOk = stat(workFolder, &st) == 0;
    if (statOk && S_ISDIR(st.st_mode))
    {
        int ret = ADUC_SystemUtils_RmDirRecursive(workFolder);
        if (ret != 0)
        {
            // Not a fatal error.
            Log_Error("Unable to remove sandbox, error %d", ret);
        }
    }
    else
    {
        Log_Info("Can not access folder '%s', or doesn't exist. Ignored...", workFolder);
    }
}
