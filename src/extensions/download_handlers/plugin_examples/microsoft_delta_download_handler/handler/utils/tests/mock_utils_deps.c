/**
 * @file mock_utils_deps.c
 * @brief Link-time mock implementations for microsoft_delta_download_handler_utils.c.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "mock_utils_deps.h"

#include <aduc/extension_manager_download_options.h>
#include <aduc/result.h>
#include <aduc/types/download.h>
#include <aduc/types/update_content.h>
#include <azure_c_shared_utility/crt_abstractions.h>
#include <azure_c_shared_utility/strings.h>
#include <stdlib.h>
#include <string.h>

struct MockUtilsDepsState g_mockUtilsDeps;

ExtensionManager_Download_Options Default_ExtensionManager_Download_Options = { 480 };

void ResetUtilsMocks(void)
{
    memset(&g_mockUtilsDeps, 0, sizeof(g_mockUtilsDeps));
}

ADUC_Result ADUC_SourceUpdateCache_Lookup(
    const char* updateIdProvider,
    const char* sourceUpdateHash,
    const char* sourceUpdateAlgorithm,
    const char* updateCacheBasePath,
    STRING_HANDLE* outSourceUpdatePath)
{
    (void)updateIdProvider;
    (void)sourceUpdateHash;
    (void)sourceUpdateAlgorithm;
    (void)updateCacheBasePath;

    g_mockUtilsDeps.lookupCallCount++;

    if (g_mockUtilsDeps.lookupOutPath != NULL)
    {
        *outSourceUpdatePath = g_mockUtilsDeps.lookupOutPath;
    }

    return g_mockUtilsDeps.lookupResult;
}

ADUC_Result workflow_get_expected_update_id(ADUC_WorkflowHandle handle, ADUC_UpdateId** updateId)
{
    (void)handle;
    g_mockUtilsDeps.getUpdateIdCallCount++;

    if (IsAducResultCodeFailure(g_mockUtilsDeps.getUpdateIdResult.ResultCode))
    {
        return g_mockUtilsDeps.getUpdateIdResult;
    }

    *updateId = (ADUC_UpdateId*)calloc(1, sizeof(ADUC_UpdateId));
    if (*updateId == NULL)
    {
        ADUC_Result result = { ADUC_Result_Failure };
        return result;
    }

    if (g_mockUtilsDeps.updateIdProvider != NULL)
    {
        mallocAndStrcpy_s(&(*updateId)->Provider, g_mockUtilsDeps.updateIdProvider);
    }
    if (g_mockUtilsDeps.updateIdName != NULL)
    {
        mallocAndStrcpy_s(&(*updateId)->Name, g_mockUtilsDeps.updateIdName);
    }
    if (g_mockUtilsDeps.updateIdVersion != NULL)
    {
        mallocAndStrcpy_s(&(*updateId)->Version, g_mockUtilsDeps.updateIdVersion);
    }

    return g_mockUtilsDeps.getUpdateIdResult;
}

void workflow_free_update_id(ADUC_UpdateId* updateId)
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

char* workflow_get_workfolder(const ADUC_WorkflowHandle handle)
{
    (void)handle;
    g_mockUtilsDeps.getWorkFolderCallCount++;

    if (g_mockUtilsDeps.workFolder == NULL)
    {
        return NULL;
    }

    char* copy = NULL;
    mallocAndStrcpy_s(&copy, g_mockUtilsDeps.workFolder);
    return copy;
}

ADUC_Result ExtensionManager_Download(
    const ADUC_FileEntity* entity,
    ADUC_WorkflowHandle workflowHandle,
    ExtensionManager_Download_Options* options,
    ADUC_DownloadProgressCallback downloadProgressCallback)
{
    (void)entity;
    (void)workflowHandle;
    (void)options;
    (void)downloadProgressCallback;

    g_mockUtilsDeps.downloadCallCount++;
    return g_mockUtilsDeps.downloadResult;
}
