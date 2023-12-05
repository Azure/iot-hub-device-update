/**
 * @file adu_processupdate.cpp
 * @brief The bare essential code from gen1 to allow processing of updates on the device
 * and interoperating with gen2 mechanisms.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adu_processupdate.h"

// gen2 specific
#include <aduc/adu_upd.h>
#include <aduc/adu_upd_utils.h>
#include <aduc/agent_state_store.h>
#include <aduc/workqueue.h>
#include <aduc/retry_utils.h>

// for integration with gen1 schemas, mechanisms, and back-compat
#include <aduc/adu_integration.h>
#include <aduc/workflow_data_utils.h>
#include <aduc/workflow_utils.h>
#include <aduc/content_handler.hpp>
#include <aduc/extension_manager.hpp>

// utils
#include <aduc/calloc_wrapper.hpp>
#include <aduc/string_c_utils.h>

void Adu_ProcessUpdate(ADUC_Update_Request_Operation_Data* updateData, ADUC_Retriable_Operation_Context* retriableOperationContext)
{
    Log_Debug("Handle Processing");

    ADUC_WorkflowHandle workqueueHandle = ADUC_StateStore_GetUpdateWorkQueueHandle();
    WorkQueueItemHandle itemHandle = WorkQueue_GetNextWork(workqueueHandle);
    char* jsonPayload = WorkQueueItem_GetUpdateResultMessageJson(itemHandle);
    int updateManifestVersion = 0;
    ContentHandler* contentHandler = nullptr;
    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));

    const auto reportFailureFn = [retriableOperationContext, updateData](int32_t erc)
    {
        retriableOperationContext->lastFailureTime = ADUC_GetTimeSinceEpochInSeconds();
        updateData->resultCode = ADUC_Result_Failure;
        updateData->extendedResultCode = erc;
        AduUpdUtils_TransitionState(ADU_UPD_STATE_REPORT_RESULTS, updateData);
    };

    // Note: Most of this logic was ported over from agent_workflow.c
    ADUC_WorkflowHandle nextWorkflow = {0};
    ADUC_Result result = workflow_init(jsonPayload, true /* shouldValidate */, &nextWorkflow);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("workflow_init failed, rc:%d, erc:%d", result.ResultCode, result.ExtendedResultCode);
        reportFailureFn(result.ExtendedResultCode);
        goto done;
    }

    if (updateData->curWorkflow != nullptr && workflow_id_compare(updateData->curWorkflow, nextWorkflow))
    {
        // same workflow. TODO: consider support of retry, ignore for now.
        Log_Debug("new workflow id matches '%s'. Ignoring.", workflow_peek_id(nextWorkflow));
        AduUpdUtils_TransitionState(ADU_UPD_STATE_IDLEWAIT, updateData);
        goto done;
    }

    // Continue with the new workflow.
    if (updateData->curWorkflow != nullptr)
    {
        workflow_free(updateData->curWorkflow);
    }

    workflowData.WorkflowHandle = nextWorkflow;
    updateData->curWorkflow = nextWorkflow;
    nextWorkflow = nullptr;

    // Check update manifest version
    updateManifestVersion = workflow_get_update_manifest_version(workflowData.WorkflowHandle);
    if (updateManifestVersion < 5)
    {
        reportFailureFn(ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_UNSUPPORTED_UPDATE_MANIFEST_VERSION);
        goto done;
    }

    // Load the content handler extension
    {
        ADUC::StringUtils::cstr_wrapper updateManifestHandler{ ADUC_StringFormat(
                "microsoft/update-manifest:%d", updateManifestVersion) };

        ADUC_Result loadResult = ExtensionManager::LoadUpdateContentHandlerExtension(updateManifestHandler.get(), &contentHandler);
        if (IsAducResultCodeFailure(loadResult.ResultCode))
        {
            reportFailureFn(ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_UNSUPPORTED_UPDATE_MANIFEST_VERSION);
            goto done;
        }
    }

    // Check if the update is already installed.
    result = contentHandler->IsInstalled(&workflowData);
    if (result.ResultCode == ADUC_Result_IsInstalled_Installed)
    {
        char* updateId = workflow_get_expected_update_id_string(workflowData.WorkflowHandle);
        ADU_Integration_SetInstalledUpdateIdAndGoToIdle(&workflowData, updateId);
        free(updateId);

        Log_Info("update already installed. Ignoring. workflowId: '%s'", workflow_peek_id(nextWorkflow));
        updateData->resultCode = 0;
        updateData->extendedResultCode = 0;
        AduUpdUtils_TransitionState(ADU_UPD_STATE_REPORT_RESULTS, updateData);
        goto done;
    }


done:
    return;
}
