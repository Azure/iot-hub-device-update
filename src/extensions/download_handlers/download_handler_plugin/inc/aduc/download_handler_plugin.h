#ifndef DOWNLOAD_HANDLER_PLUGIN_H
#define DOWNLOAD_HANDLER_PLUGIN_H

#include <aduc/c_utils.h> // EXTERN_C_*
#include <aduc/result.h> // ADUC_Result
#include <aduc/types/update_content.h> // ADUC_FileEntity
#include <aduc/types/workflow.h> // ADUC_FileEntity

EXTERN_C_BEGIN

typedef void* DownloadHandlerHandle;

/**
 * @brief Called when the update workflow successfully completes.
 * In the case of Delta download handler plugin, it moves the file at the payloadFilePath to the cache.
 *
 * @param[in] handle The handle to the download handler.
 * @param[in] workflowHandle The workflow handle.
 * @return ADUC_Result The result.
 */
ADUC_Result ADUC_DownloadHandlerPlugin_OnUpdateWorkflowCompleted(
    const DownloadHandlerHandle handle, const ADUC_WorkflowHandle workflowHandle);

EXTERN_C_END

#endif // DOWNLOAD_HANDLER_PLUGIN_H
