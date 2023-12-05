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
    char* updateId = nullptr;
    ADUC_WorkflowHandle nextWorkflow = nullptr;
    ADUC_Result result;
    memset(&result, 0, sizeof(result));
    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));

    const auto reportFailureFn = [retriableOperationContext, updateData, &updateId, &workflowData, &result](int32_t erc)
    {
        retriableOperationContext->lastFailureTime = ADUC_GetTimeSinceEpochInSeconds();
        updateData->resultCode = ADUC_Result_Failure;
        updateData->extendedResultCode = erc;

        result.ResultCode = ADUC_Result_Failure;
        result.ExtendedResultCode = erc;
        updateId = workflow_get_expected_update_id_string(workflowData.WorkflowHandle);
        updateData->reportingJson = ADU_Integration_GetReportingJson(
            &workflowData,
            ADUCITF_State_Idle,
            &result,
            updateId);
        free(updateId);

        AduUpdUtils_TransitionState(ADU_UPD_STATE_REPORT_RESULTS, updateData);
    };

    if (std::string{jsonPayload} == "{}")
    {
        Log_Info("        *** ADU service had no applicable updates.");
        AduUpdUtils_TransitionState(ADU_UPD_STATE_IDLEWAIT, updateData);
        goto done;
    }

    // Note: Most of this logic was ported over from agent_workflow.c
    nextWorkflow = nullptr;
    result = workflow_init(jsonPayload, true /* shouldValidate */, &nextWorkflow);
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

    workflowData.CommunicationChannel = ADUC_CommunicationChannelType_MQTTBroker;

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
        Log_Info("update already installed. Ignoring. workflowId: '%s'", workflow_peek_id(nextWorkflow));
        updateData->resultCode = result.ResultCode;
        updateData->extendedResultCode = result.ExtendedResultCode;
        retriableOperationContext->lastSuccessTime = ADUC_GetTimeSinceEpochInSeconds();

        result.ResultCode = result.ResultCode;
        result.ExtendedResultCode = result.ExtendedResultCode;

        updateId = workflow_get_expected_update_id_string(workflowData.WorkflowHandle);
        updateData->reportingJson = ADU_Integration_GetReportingJson(
            &workflowData,
            ADUCITF_State_Idle,
            &result,
            updateId);
        free(updateId);

        AduUpdUtils_TransitionState(ADU_UPD_STATE_REPORT_RESULTS, updateData);
        goto done;
    }
    else if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("Failed IsInstalled, erc: %d", result.ExtendedResultCode);
        reportFailureFn(result.ExtendedResultCode);
        goto done;
    }

    result = contentHandler->Download(&workflowData);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("Failed Download, erc: %d", result.ExtendedResultCode);
        reportFailureFn(result.ExtendedResultCode);
        goto done;
    }

    result = contentHandler->Install(&workflowData);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("Failed Install, erc: %d", result.ExtendedResultCode);
        reportFailureFn(result.ExtendedResultCode);
        goto done;
    }

    result = contentHandler->Apply(&workflowData);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("Failed Apply, erc: %d", result.ExtendedResultCode);
        reportFailureFn(result.ExtendedResultCode);
        goto done;
    }

    Log_Info("Apply Succeeded. Reporting results, resultCode: %d", result.ResultCode);

    updateData->resultCode = result.ResultCode;
    updateData->extendedResultCode = result.ExtendedResultCode;


    // TODO: push onto reporting work queue instead of in operation data.
    updateId = workflow_get_expected_update_id_string(workflowData.WorkflowHandle);
    updateData->reportingJson = ADU_Integration_GetReportingJson(
        &workflowData,
        ADUCITF_State_Idle,
        &result,
        updateId);
    free(updateId);

    if (IsNullOrEmpty(updateData->reportingJson))
    {
        Log_Error("Failed generating reporting json");
        reportFailureFn(ADUC_ERC_NOMEM);
        goto done;
    }

    // The reporting json is set in the update operation data, so now we transition
    // to the report results state where it will publish it to the broker.
    retriableOperationContext->attemptCount = 0;
    retriableOperationContext->nextExecutionTime = ADUC_GetTimeSinceEpochInSeconds() + 5;
    retriableOperationContext->expirationTime = retriableOperationContext->nextExecutionTime + 60 * 60; // TODO: make configurable
    AduUpdUtils_TransitionState(ADU_UPD_STATE_REPORT_RESULTS, updateData);

done:
    return;
}
