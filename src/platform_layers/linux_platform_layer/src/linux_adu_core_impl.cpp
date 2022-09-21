/**
 * @file linux_adu_core_impl.cpp
 * @brief Implements an ADUC reference implementation library.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "linux_adu_core_impl.hpp"
#include "aduc/agent_workflow.h"
#include "aduc/calloc_wrapper.hpp"
#include "aduc/content_handler.hpp"
#include "aduc/extension_manager.hpp"
#include "aduc/hash_utils.h"
#include "aduc/string_c_utils.h"
#include "aduc/string_utils.hpp"
#include "aduc/system_utils.h"
#include "aduc/types/workflow.h"
#include "aduc/workflow_data_utils.h"
#include "aduc/workflow_internal.h"
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
using ADUC::StringUtils::cstr_wrapper;

// The update manifest handler type.
#define UPDATE_MANIFEST_DEFAULT_HANDLER "microsoft/update-manifest"

std::string LinuxPlatformLayer::g_componentsInfo;
time_t LinuxPlatformLayer::g_lastComponentsCheckTime;

/**
 * @brief Factory method for LinuxPlatformLayer
 * @return std::unique_ptr<LinuxPlatformLayer> The newly created LinuxPlatformLayer object.
 */
std::unique_ptr<LinuxPlatformLayer> LinuxPlatformLayer::Create()
{
    struct timeval tv
    {
    };
    gettimeofday(&tv, nullptr);
    g_lastComponentsCheckTime = tv.tv_sec;

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
    data->BackupCallback = BackupCallback;
    data->InstallCallback = InstallCallback;
    data->ApplyCallback = ApplyCallback;
    data->RestoreCallback = RestoreCallback;
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

static ContentHandler* GetUpdateManifestHandler(const ADUC_WorkflowData* workflowData, ADUC_Result* result)
{
    ContentHandler* contentHandler = nullptr;

    ADUC_Result loadResult = {};

    // Starting from version 4, the top-level update manifest doesn't contains the 'updateType' property.
    // The manifest contains an Instruction (steps) data, which requires special processing.
    // For backword compatibility and avoid code complexity, for V4, we will process the top level update content
    // using 'microsoft/update-manifest:4'
    int updateManifestVersion = workflow_get_update_manifest_version(workflowData->WorkflowHandle);
    if (updateManifestVersion >= 4)
    {
        const cstr_wrapper updateManifestHandler{ ADUC_StringFormat(
            "microsoft/update-manifest:%d", updateManifestVersion) };

        Log_Info(
            "Try to load a handler for current update manifest version %d (handler: '%s')",
            updateManifestVersion,
            updateManifestHandler.get());

        loadResult =
            ExtensionManager::LoadUpdateContentHandlerExtension(updateManifestHandler.get(), &contentHandler);

        // If handler for the current manifest version is not available,
        // fallback to the V4 default handler.
        if (IsAducResultCodeFailure(loadResult.ResultCode))
        {
            loadResult = ExtensionManager::LoadUpdateContentHandlerExtension(
                UPDATE_MANIFEST_DEFAULT_HANDLER, &contentHandler);
        }
    }
    else
    {
        loadResult = { .ResultCode = ADUC_Result_Failure,
                       .ExtendedResultCode =
                           ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_UNSUPPORTED_UPDATE_MANIFEST_VERSION };
    }

    if (IsAducResultCodeFailure(loadResult.ResultCode))
    {
        contentHandler = nullptr;
        *result = loadResult;
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
    ContentHandler* contentHandler = GetUpdateManifestHandler(workflowData, &result);

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
    return result;
}

/**
 * @brief Class implementation of Install method.
 * @return ADUC_Result
 */
ADUC_Result LinuxPlatformLayer::Install(const ADUC_WorkflowData* workflowData)
{
    ADUC_Result result{ ADUC_Result_Failure };
    ContentHandler* contentHandler = GetUpdateManifestHandler(workflowData, &result);
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
    return result;
}

/**
 * @brief Class implementation of Apply method.
 * @return ADUC_Result
 */
ADUC_Result LinuxPlatformLayer::Apply(const ADUC_WorkflowData* workflowData)
{
    ADUC_Result result{ ADUC_Result_Failure };
    ContentHandler* contentHandler = GetUpdateManifestHandler(workflowData, &result);
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
    return result;
}

/**
 * @brief Class implementation of Backup method.
 * @return ADUC_Result
 */
ADUC_Result LinuxPlatformLayer::Backup(const ADUC_WorkflowData* workflowData)
{
    ADUC_Result result{ ADUC_Result_Failure };
    ContentHandler* contentHandler = GetUpdateManifestHandler(workflowData, &result);
    if (contentHandler == nullptr)
    {
        goto done;
    }

    result = contentHandler->Backup(workflowData);

    // If cancel is requested during backup, we will proceed to finish the backup.
    if (_IsCancellationRequested)
    {
        result = ADUC_Result{ ADUC_Result_Failure_Cancelled };
        _IsCancellationRequested = false; // For replacement, we can't call Idle so reset here
    }

done:
    return result;
}
/**
 * @brief Class implementation of Restore method.
 * @return ADUC_Result
 */
ADUC_Result LinuxPlatformLayer::Restore(const ADUC_WorkflowData* workflowData)
{
    ADUC_Result result{ ADUC_Result_Failure };
    ContentHandler* contentHandler = GetUpdateManifestHandler(workflowData, &result);
    if (contentHandler == nullptr)
    {
        goto done;
    }

    result = contentHandler->Restore(workflowData);

    // If cancel is requested during restore, it means that the user wants to cancel the deployment (which already failed),
    // so the agent should try to restore to the previous state - proceed to finish the restore.

done:
    return result;
}

/**
 * @brief Class implementation of Cancel method.
 */
void LinuxPlatformLayer::Cancel(const ADUC_WorkflowData* workflowData)
{
    ADUC_Result result{ ADUC_Result_Failure };

    _IsCancellationRequested = true;

    ContentHandler* contentHandler = GetUpdateManifestHandler(workflowData, &result);
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
        Log_Info("Successful cancel of workflowId: %s", workflow_peek_id(workflowData->WorkflowHandle));
    }
    else
    {
        Log_Warn("Failed to cancel workflowId: %s", workflow_peek_id(workflowData->WorkflowHandle));
    }

done:
    return;
}

/**
 * @brief Class implementation of the IsInstalled callback.
 * Calls into the content handler or step handler to determine if the update in the given workflow
 * is installed.
 *
 * @param workflowData The workflow data object.
 * @return ADUC_Result The result of the IsInstalled call.
 */
ADUC_Result LinuxPlatformLayer::IsInstalled(const ADUC_WorkflowData* workflowData)
{
    ContentHandler* contentHandler = nullptr;

    if (workflowData == nullptr)
    {
        return ADUC_Result{ .ResultCode = ADUC_Result_Failure,
                            .ExtendedResultCode = ADUC_ERC_UPDATE_CONTENT_HANDLER_ISINSTALLED_FAILURE_NULL_WORKFLOW };
    }

    ADUC_Result result;
    contentHandler = GetUpdateManifestHandler(workflowData, &result);
    if (contentHandler == nullptr)
    {
        return ADUC_Result{ .ResultCode = ADUC_Result_Failure,
                            .ExtendedResultCode = ADUC_ERC_UPDATE_CONTENT_HANDLER_ISINSTALLED_FAILURE_BAD_UPDATETYPE };
    }

    return contentHandler->IsInstalled(workflowData);
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
