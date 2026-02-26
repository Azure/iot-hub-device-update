/**
 * @file mock_utils_deps.h
 * @brief Configurable mock state for microsoft_delta_download_handler_utils.c dependencies.
 *
 * Link-time mocks for:
 *   ADUC_SourceUpdateCache_Lookup,
 *   workflow_get_expected_update_id / workflow_free_update_id,
 *   workflow_get_workfolder,
 *   ExtensionManager_Download + Default_ExtensionManager_Download_Options.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef MOCK_UTILS_DEPS_H
#define MOCK_UTILS_DEPS_H

#include <aduc/result.h>
#include <aduc/types/update_content.h>
#include <aduc/types/workflow.h>
#include <azure_c_shared_utility/strings.h>

#ifdef __cplusplus
extern "C"
{
#endif

    struct MockUtilsDepsState
    {
        /* ADUC_SourceUpdateCache_Lookup mock */
        ADUC_Result lookupResult;
        STRING_HANDLE lookupOutPath;   /**< STRING_HANDLE to return as *outSourceUpdatePath */
        int lookupCallCount;

        /* workflow_get_expected_update_id mock */
        ADUC_Result getUpdateIdResult;
        char* updateIdProvider;
        char* updateIdName;
        char* updateIdVersion;
        int getUpdateIdCallCount;

        /* workflow_get_workfolder mock */
        char* workFolder;             /**< Returned by workflow_get_workfolder; caller will free() */
        int getWorkFolderCallCount;

        /* ExtensionManager_Download mock */
        ADUC_Result downloadResult;
        int downloadCallCount;
    };

    extern struct MockUtilsDepsState g_mockUtilsDeps;
    void ResetUtilsMocks(void);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_UTILS_DEPS_H */
