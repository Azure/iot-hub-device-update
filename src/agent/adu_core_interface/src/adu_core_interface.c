/**
 * @file adu_core_interface.c
 * @brief Methods to communicate with "urn:azureiot:AzureDeviceUpdateCore:1" interface.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adu_core_interface.h"
#include "aduc/adu_core_export_helpers.h" // ADUC_Workflow_SetUpdateStateWithResult
#include "aduc/agent_orchestration.h"
#include "aduc/agent_workflow.h"
#include "aduc/c_utils.h"
#include "aduc/client_handle_helper.h"
#include "aduc/config_utils.h"
#include "aduc/d2c_messaging.h"
#include "aduc/hash_utils.h"
#include "aduc/logging.h"
#include "aduc/reporting_utils.h"
#include "aduc/rootkey_workflow.h"
#include "aduc/rootkeypackage_do_download.h"
#include "aduc/rootkeypackage_types.h"
#include "aduc/rootkeypackage_utils.h"
#include "aduc/string_c_utils.h"
#include "aduc/types/adu_core.h"
#include "aduc/types/update_content.h"
#include "aduc/workflow_data_utils.h"
#include "aduc/workflow_utils.h"

#include "startup_msg_helper.h"

#include <azure_c_shared_utility/strings.h> // STRING_*
#include <iothub_client_version.h>
#include <parson.h>
#include <pnp_protocol.h>

// Name of an Device Update Agent component that this device implements.
static const char g_aduPnPComponentName[] = "deviceUpdate";

// Name of properties that Device Update Agent component supports.

// This is the device-to-cloud property.
// An agent communicates its state and other data to ADU Management service by reporting this property to IoTHub.
static const char g_aduPnPComponentAgentPropertyName[] = "agent";

// This is the cloud-to-device property.
// ADU Management send an 'Update Action' to this device by setting this property on IoTHub.
static const char g_aduPnPComponentServicePropertyName[] = "service";

/**
 * @brief Handle for Device Update Agent component to communication to service.
 */
ADUC_ClientHandle g_iotHubClientHandleForADUComponent;

/**
 * @brief This function is called when the message is no longer being process.
 *
 * @param context The ADUC_D2C_Message object
 * @param status The message status.
 */
static void OnUpdateResultD2CMessageCompleted(void* context, ADUC_D2C_Message_Status status)
{
    UNREFERENCED_PARAMETER(context);
    Log_Debug("Send message completed (status:%d)", status);
}

/**
 * @brief Initialize a ADUC_WorkflowData object.
 *
 * @param[out] workflowData Workflow metadata.
 * @param argc Count of arguments in @p argv
 * @param argv Command line parameters.
 * @return bool True on success.
 */
bool ADUC_WorkflowData_Init(ADUC_WorkflowData* workflowData, int argc, char** argv)
{
    bool succeeded = false;

    memset(workflowData, 0, sizeof(*workflowData));

    ADUC_Result result = ADUC_MethodCall_Register(&(workflowData->UpdateActionCallbacks), argc, (const char**)argv);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("ADUC_RegisterPlatformLayer failed %d, %d", result.ResultCode, result.ExtendedResultCode);
        goto done;
    }

    // Only call Unregister if register succeeded.
    workflowData->IsRegistered = true;

    workflowData->DownloadProgressCallback = ADUC_Workflow_DefaultDownloadProgressCallback;

    workflowData->ReportStateAndResultAsyncCallback = AzureDeviceUpdateCoreInterface_ReportStateAndResultAsync;

    workflowData->LastCompletedWorkflowId = NULL;

    workflow_set_cancellation_type(workflowData->WorkflowHandle, ADUC_WorkflowCancellationType_None);

    succeeded = true;

done:
    return succeeded;
}

/**
 * @brief Free members of ADUC_WorkflowData object.
 *
 * @param workflowData Object whose members should be freed.
 */
void ADUC_WorkflowData_Uninit(ADUC_WorkflowData* workflowData)
{
    if (workflowData == NULL)
    {
        return;
    }

    if (workflowData->IsRegistered)
    {
        ADUC_MethodCall_Unregister(&(workflowData->UpdateActionCallbacks));
    }

    workflow_free_string(workflowData->LastCompletedWorkflowId);
    memset(workflowData, 0, sizeof(*workflowData));
}

/**
 * @brief Reports the client json via PnP so it ends up in the reported section of the twin.
 *
 * @param messageType The message type.
 * @param json_value The json value to be reported.
 * @param workflowData The workflow data.
 * @return bool true if call succeeded.
 */
static bool
ReportClientJsonProperty(ADUC_D2C_Message_Type messageType, const char* json_value, ADUC_WorkflowData* workflowData)
{
    UNREFERENCED_PARAMETER(workflowData);

    bool success = false;

    if (g_iotHubClientHandleForADUComponent == NULL)
    {
        Log_Error("ReportClientJsonProperty called with invalid IoTHub Device Client handle! Can't report!");
        return false;
    }

    STRING_HANDLE jsonToSend =
        PnP_CreateReportedProperty(g_aduPnPComponentName, g_aduPnPComponentAgentPropertyName, json_value);

    if (jsonToSend == NULL)
    {
        Log_Error("Unable to create Reported property for ADU client.");
        goto done;
    }

    if (!ADUC_D2C_Message_SendAsync(
            messageType,
            &g_iotHubClientHandleForADUComponent,
            STRING_c_str(jsonToSend),
            NULL /* responseCallback */,
            OnUpdateResultD2CMessageCompleted,
            NULL /* statusChangedCallback */,
            NULL /* userData */))
    {
        Log_Error("Unable to send update result.");
        goto done;
    }

    success = true;

done:
    STRING_delete(jsonToSend);

    return success;
}

/**
 * @brief Reports values to the cloud which do not change throughout ADUs execution
 * @details the current expectation is to report these values after the successful
 * connection of the AzureDeviceUpdateCoreInterface
 * @param workflowData the workflow data.
 * @returns true when the report is sent and false when reporting fails.
 */
bool ReportStartupMsg(ADUC_WorkflowData* workflowData)
{
    if (g_iotHubClientHandleForADUComponent == NULL)
    {
        Log_Error("ReportStartupMsg called before registration! Can't report!");
        return false;
    }

    bool success = false;
    const ADUC_ConfigInfo* config = NULL;
    char* jsonString = NULL;

    JSON_Value* startupMsgValue = json_value_init_object();

    if (startupMsgValue == NULL)
    {
        goto done;
    }

    JSON_Object* startupMsgObj = json_value_get_object(startupMsgValue);

    if (startupMsgObj == NULL)
    {
        goto done;
    }

    config = ADUC_ConfigInfo_GetInstance();

    if (config == NULL)
    {
        goto done;
    }

    const ADUC_AgentInfo* agent = ADUC_ConfigInfo_GetAgent(config, 0);

    if (!StartupMsg_AddDeviceProperties(startupMsgObj, agent))
    {
        Log_Error("Could not add Device Properties to the startup message");
        goto done;
    }

    if (!StartupMsg_AddCompatPropertyNames(startupMsgObj))
    {
        Log_Error("Could not add compatPropertyNames to the startup message");
        goto done;
    }

    jsonString = json_serialize_to_string(startupMsgValue);

    if (jsonString == NULL)
    {
        Log_Error("Serializing JSON to string failed!");
        goto done;
    }

    ReportClientJsonProperty(ADUC_D2C_Message_Type_Device_Properties, jsonString, workflowData);

    success = true;
done:
    json_value_free(startupMsgValue);
    json_free_serialized_string(jsonString);
    ADUC_ConfigInfo_ReleaseInstance(config);
    return success;
}

//
// AzureDeviceUpdateCoreInterface methods
//

bool AzureDeviceUpdateCoreInterface_Create(void** context, int argc, char** argv)
{
    bool succeeded = false;

    ADUC_WorkflowData* workflowData = calloc(1, sizeof(ADUC_WorkflowData));
    if (workflowData == NULL)
    {
        goto done;
    }

    Log_Info("ADUC agent started. Using IoT Hub Client SDK %s", IoTHubClient_GetVersionString());

    if (!ADUC_WorkflowData_Init(workflowData, argc, argv))
    {
        Log_Error("Workflow data initialization failed");
        goto done;
    }

    succeeded = true;

done:

    if (!succeeded)
    {
        ADUC_WorkflowData_Uninit(workflowData);
        free(workflowData);
        workflowData = NULL;
    }

    // Set out parameter.
    *context = workflowData;

    return succeeded;
}

void AzureDeviceUpdateCoreInterface_Connected(void* componentContext)
{
    ADUC_WorkflowData* workflowData = (ADUC_WorkflowData*)componentContext;

    if (workflowData->WorkflowHandle == NULL)
    {
        // Only perform startup logic here, if no workflows has been created.
        ADUC_Workflow_HandleStartupWorkflowData(workflowData);
    }

    if (!ReportStartupMsg(workflowData))
    {
        Log_Warn("ReportStartupMsg failed");
    }
}

void AzureDeviceUpdateCoreInterface_DoWork(void* componentContext)
{
    ADUC_WorkflowData* workflowData = (ADUC_WorkflowData*)componentContext;
    ADUC_Workflow_DoWork(workflowData);
}

void AzureDeviceUpdateCoreInterface_Destroy(void** componentContext)
{
    ADUC_WorkflowData* workflowData = (ADUC_WorkflowData*)(*componentContext);

    Log_Info("ADUC agent stopping");

    ADUC_WorkflowData_Uninit(workflowData);
    free(workflowData);

    *componentContext = NULL;
}

/**
 * @brief Update twin to report state transition before workflow processing has started.
 *
 * @param propertyValue The json value to use for reporting.
 * @param deploymentState The final deployment state to report.
 * @param workflowData The workflow data to receive the last reported state upon reporting success.
 * @param result The result to be reported.
 * @return true on reporting success.
 */
static bool ReportPreDeploymentProcessingState(
    JSON_Value* propertyValue, ADUCITF_State deploymentState, ADUC_WorkflowData* workflowData, ADUC_Result result)
{
    JSON_Value* propertyValueCopy = NULL;
    bool reportingSuccess = false;

    // Temp workflowData and workflow handle for reporting
    ADUC_WorkflowData tmpWorkflowData;
    memset(&tmpWorkflowData, 0, sizeof(tmpWorkflowData));

    if (!ADUC_WorkflowData_InitWorkflowHandle(&tmpWorkflowData))
    {
        goto done;
    }

    // Synthesize workflowData current action and set a copy of the
    // propertyValue to workflow UpdateActionObject, both of which are
    // needed to generate the reporting json.
    tmpWorkflowData.CurrentAction = ADUCITF_UpdateAction_ProcessDeployment;
    propertyValueCopy = json_value_deep_copy(propertyValue);
    if (propertyValueCopy == NULL)
    {
        goto done;
    }

    if (!workflow_set_update_action_object(tmpWorkflowData.WorkflowHandle, json_object(propertyValueCopy)))
    {
        goto done;
    }

    reportingSuccess = AzureDeviceUpdateCoreInterface_ReportStateAndResultAsync(
        (ADUC_WorkflowDataToken)&tmpWorkflowData, deploymentState, &result, NULL /* installedUpdateId */);
    if (!reportingSuccess)
    {
        goto done;
    }

    // Set the last deployment state on the actual workflow data for correct handling of update action.
    ADUC_WorkflowData_SetLastReportedState(deploymentState, workflowData);

    reportingSuccess = true;
done:

    if (tmpWorkflowData.WorkflowHandle != NULL)
    {
        // propertyValueCopy will get freed by workflow_free
        workflow_free(tmpWorkflowData.WorkflowHandle);
    }

    return reportingSuccess;
}

/**
 * @brief Callback for the orchestrator that allows the new patches coming down from the cloud to be organized
 * @param clientHandle the client handle being used for the connection
 * @param propertyValue the value of the property being routed
 * @param propertyVersion the version of the property being routed
 * @param sourceContext the context of the origination point for the callback
 * @param context context for re-entering upon completion of the function
 */
void OrchestratorUpdateCallback(
    ADUC_ClientHandle clientHandle,
    JSON_Value* propertyValue,
    int propertyVersion,
    ADUC_PnPComponentClient_PropertyUpdate_Context* sourceContext,
    void* context)
{
    UNREFERENCED_PARAMETER(clientHandle);

    ADUC_Result tmpResult = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    ADUC_WorkflowData* workflowData = (ADUC_WorkflowData*)context;

    STRING_HANDLE jsonToSend = NULL;
    char* ackString = NULL;
    JSON_Object* signatureObj = NULL;

    ADUCITF_UpdateAction updateAction = ADUCITF_UpdateAction_Undefined;
    char* workflowId = NULL;
    char* rootKeyPkgUrl = NULL;
    STRING_HANDLE rootKeyPackageFilePath = NULL;
    char* workFolder = NULL;

    // Reads out the json string so we can Log Out what we've got.
    // The value will be parsed and handled in ADUC_Workflow_HandlePropertyUpdate.
    char* jsonString = json_serialize_to_string(propertyValue);
    if (jsonString == NULL)
    {
        Log_Error(
            "OrchestratorUpdateCallback failed to convert property JSON value to string, property version (%d)",
            propertyVersion);
        goto done;
    }

    // To reduce TWIN size, remove UpdateManifestSignature and fileUrls before ACK.
    signatureObj = json_value_get_object(propertyValue);
    if (signatureObj != NULL)
    {
        json_object_set_null(signatureObj, "updateManifestSignature");
        json_object_set_null(signatureObj, "fileUrls");
        ackString = json_serialize_to_string(propertyValue);
    }

    Log_Debug("Update Action info string (%s), property version (%d)", ackString, propertyVersion);

    tmpResult = workflow_parse_peek_unprotected_workflow_properties(
        json_object(propertyValue), &updateAction, &rootKeyPkgUrl, &workflowId);
    if (IsAducResultCodeFailure(tmpResult.ResultCode))
    {
        Log_Error("Parse failed for unprotected properties, erc: 0x%08x", tmpResult.ExtendedResultCode);
        // Note, cannot report failure here since workflowId from unprotected properties is needed for that.
        goto done;
    }

    if (updateAction == ADUCITF_UpdateAction_ProcessDeployment && !IsNullOrEmpty(workflowId))
    {
        Log_Debug("Processing deployment %s ...", workflowId);

        ADUC_Result inProgressResult = { .ResultCode = ADUC_GeneralResult_Success, .ExtendedResultCode = 0 };
        if (!ReportPreDeploymentProcessingState(
                propertyValue, ADUCITF_State_DeploymentInProgress, workflowData, inProgressResult))
        {
            Log_Warn("Reporting InProgress failed. Continuing processing deployment %s", workflowId);
        }

        // Ensure update to latest rootkey pkg, which is required for validating the update metadata.
        workFolder = workflow_get_root_sandbox_dir(workflowData->WorkflowHandle);
        if (workFolder == NULL)
        {
            Log_Error("workflow_get_root_sandbox_dir failed");
            goto done;
        }

        tmpResult = RootKeyWorkflow_UpdateRootKeys(workflowId, workFolder, rootKeyPkgUrl);
        if (IsAducResultCodeFailure(tmpResult.ResultCode))
        {
            Log_Error("Update Rootkey failed, 0x%08x. Deployment cannot proceed.", tmpResult.ExtendedResultCode);

            if (!ReportPreDeploymentProcessingState(propertyValue, ADUCITF_State_Failed, workflowData, tmpResult))
            {
                Log_Warn("FAIL: report rootkey update 'Failed' State.");
            }

            goto done;
        }
    }

    ADUC_Workflow_HandlePropertyUpdate(workflowData, (const unsigned char*)jsonString, sourceContext->forceUpdate);
    free(jsonString);
    jsonString = ackString;

    // ACK the request.
    jsonToSend = PnP_CreateReportedPropertyWithStatus(
        g_aduPnPComponentName,
        g_aduPnPComponentServicePropertyName,
        jsonString,
        PNP_STATUS_SUCCESS,
        "", // Description for this acknowledgement.
        propertyVersion);

    if (jsonToSend == NULL)
    {
        Log_Error("Unable to build reported property ACK response.");
        goto done;
    }

    if (!ADUC_D2C_Message_SendAsync(
            ADUC_D2C_Message_Type_Device_Update_ACK,
            &g_iotHubClientHandleForADUComponent,
            STRING_c_str(jsonToSend),
            NULL /* responseCallback */,
            OnUpdateResultD2CMessageCompleted,
            NULL /* statusChangedCallback */,
            NULL /* userData */))
    {
        Log_Error("Unable to send update result.");
        goto done;
    }

done:
    STRING_delete(rootKeyPackageFilePath);
    workflow_free_string(rootKeyPkgUrl);
    workflow_free_string(workflowId);
    workflow_free_string(workFolder);
    STRING_delete(jsonToSend);
    free(jsonString);

    Log_Info("OrchestratorPropertyUpdateCallback ended");
}

/**
 * @brief This function is invoked when Device Update PnP Interface property is updated.
 *
 * @param clientHandle A Device Update Client handle object.
 * @param propertyName The name of the property that changed.
 * @param propertyValue The new property value.
 * @param version Property version.
 * @param sourceContext An information about the source of the property update notificaion.
 * @param context An ADUC_WorkflowData object.
 */
void AzureDeviceUpdateCoreInterface_PropertyUpdateCallback(
    ADUC_ClientHandle clientHandle,
    const char* propertyName,
    JSON_Value* propertyValue,
    int version,
    ADUC_PnPComponentClient_PropertyUpdate_Context* sourceContext,
    void* context)
{
    if (strcmp(propertyName, g_aduPnPComponentServicePropertyName) == 0)
    {
        OrchestratorUpdateCallback(clientHandle, propertyValue, version, sourceContext, context);
    }
    else
    {
        Log_Info("Unsupported property. (%s)", propertyName);
    }
}

//
// Reporting
//
static JSON_Status _json_object_set_update_result(
    JSON_Object* object, int32_t resultCode, STRING_HANDLE extendedResultCodes, const char* resultDetails)
{
    JSON_Status status = json_object_set_number(object, ADUCITF_FIELDNAME_RESULTCODE, resultCode);
    if (status != JSONSuccess)
    {
        Log_Error("Could not set value for field: %s", ADUCITF_FIELDNAME_RESULTCODE);
        goto done;
    }

    status = json_object_set_string(object, ADUCITF_FIELDNAME_EXTENDEDRESULTCODES, STRING_c_str(extendedResultCodes));
    if (status != JSONSuccess)
    {
        Log_Error("Could not set value for field: %s", ADUCITF_FIELDNAME_EXTENDEDRESULTCODES);
        goto done;
    }

    if (resultDetails != NULL)
    {
        status = json_object_set_string(object, ADUCITF_FIELDNAME_RESULTDETAILS, resultDetails);
        if (status != JSONSuccess)
        {
            Log_Error("Could not set value for field: %s", ADUCITF_FIELDNAME_RESULTDETAILS);
        }
    }
    else
    {
        status = json_object_set_null(object, ADUCITF_FIELDNAME_RESULTDETAILS);
        if (status != JSONSuccess)
        {
            Log_Error("Could not set field %s to 'null'", ADUCITF_FIELDNAME_RESULTDETAILS);
        }
    }

done:
    return status;
}

/**
 * @brief Sets workflow properties on the workflow json value.
 *
 * @param[in,out] workflowValue The workflow json value to set properties on.
 * @param[in] updateAction The updateAction for the action field.
 * @param[in] workflowId The workflow id of the update deployment.
 * @param[in] retryTimestamp optional. The retry timestamp that's present for service-initiated retries.
 * @return true if all properties were set successfully; false, otherwise.
 */
static bool set_workflow_properties(
    JSON_Value* workflowValue, ADUCITF_UpdateAction updateAction, const char* workflowId, const char* retryTimestamp)
{
    bool succeeded = false;

    JSON_Object* workflowObject = json_value_get_object(workflowValue);
    if (json_object_set_number(workflowObject, ADUCITF_FIELDNAME_ACTION, updateAction) != JSONSuccess)
    {
        Log_Error("Could not add JSON field: %s", ADUCITF_FIELDNAME_ACTION);
        goto done;
    }

    if (json_object_set_string(workflowObject, ADUCITF_FIELDNAME_ID, workflowId) != JSONSuccess)
    {
        Log_Error("Could not add JSON field: %s", ADUCITF_FIELDNAME_ID);
        goto done;
    }

    if (!IsNullOrEmpty(retryTimestamp))
    {
        if (json_object_set_string(workflowObject, ADUCITF_FIELDNAME_RETRYTIMESTAMP, retryTimestamp) != JSONSuccess)
        {
            Log_Error("Could not add JSON field: %s", ADUCITF_FIELDNAME_RETRYTIMESTAMP);
            goto done;
        }
    }

    succeeded = true;

done:

    return succeeded;
}

static STRING_HANDLE construct_extended_result_codes_str(ADUC_WorkflowHandle handle, ADUC_Result rootResult)
{
    STRING_HANDLE root_result_erc_str =
        ADUC_ReportingUtils_CreateReportingErcHexStr(rootResult.ExtendedResultCode, true /* is_first */);
    STRING_HANDLE extra_ercs_str = workflow_get_extra_ercs(handle);
    if (extra_ercs_str != NULL && STRING_length(extra_ercs_str) > 0 && root_result_erc_str != NULL
        && STRING_length(root_result_erc_str) > 0)
    {
        STRING_concat_with_STRING(root_result_erc_str, extra_ercs_str);
    }

    STRING_delete(extra_ercs_str);
    return root_result_erc_str;
}

/**
 * @brief Get the Reporting Json Value object
 *
 * @param workflowData The workflow data.
 * @param updateState The workflow state machine state.
 * @param result The pointer to the result. If NULL, then the result will be retrieved from the opaque handle object in the workflow data.
 * @param installedUpdateId The installed Update ID string.
 * @return JSON_Value* The resultant json value object. Caller must free using json_value_free().
 */
JSON_Value* GetReportingJsonValue(
    ADUC_WorkflowData* workflowData,
    ADUCITF_State updateState,
    const ADUC_Result* result,
    const char* installedUpdateId)
{
    JSON_Value* resultValue = NULL;

    // Declare and init here to avoid maybe-uninitialized static analysis errors
    JSON_Value* rootValue = NULL;
    JSON_Value* lastInstallResultValue = NULL;
    JSON_Value* stepResultsValue = NULL;
    JSON_Value* workflowValue = NULL;
    STRING_HANDLE rootResultERCs = NULL;

    //
    // Get result from current workflow if exists.
    // (Note: on startup, update workflow is not started, unless there is an existing Update Action in the twin.)
    //
    // If not, try to use specified 'result' param.
    //
    // If there's no result details, we'll report only 'updateState'.
    //
    ADUC_Result rootResult;
    ADUC_WorkflowHandle handle = workflowData->WorkflowHandle;

    if (result != NULL)
    {
        rootResult = *result;
    }
    else
    {
        rootResult = workflow_get_result(handle);
    }

    // The "extendedResultCodes" reported property is a JSON string, where the first ERC (8 hex digits) is always
    // from the rootResult. Extra ERC can be appended for soft-failing mechanisms with fallback mechanisms
    // e.g. download handler or update metadata rootkey management.
    rootResultERCs = construct_extended_result_codes_str(handle, rootResult);
    if (rootResultERCs == NULL)
    {
        goto done;
    }

    rootValue = json_value_init_object();
    JSON_Object* rootObject = json_value_get_object(rootValue);
    size_t stepsCount = workflow_get_children_count(handle);

    //
    // Prepare 'lastInstallResult', 'stepResults' data.
    //
    // Example schema:
    //
    // {
    //     "state" : ###,
    //     "workflow": {
    //         "action": 3,
    //         "id": "..."
    //     },
    //     "installedUpdateId" : "...",
    //
    //     "lastInstallResult" : {
    //         "resultCode" : ####,
    //         "extendedResultCodes" : "########,########",
    //         "resultDetails" : "...",
    //         "stepResults" : {
    //             "step_0" : {
    //                 "resultCode" : ####,
    //                 "extendedResultCodes" : "########",
    //                 "resultDetails" : "..."
    //             },
    //             ...
    //             "step_N" : {
    //                 "resultCode" : ####,
    //                 "extendedResultCodes" : "########",
    //                 "resultDetails" : "..."
    //             }
    //         }
    //     }
    // }

    lastInstallResultValue = json_value_init_object();
    JSON_Object* lastInstallResultObject = json_object(lastInstallResultValue);

    stepResultsValue = json_value_init_object();
    JSON_Object* stepResultsObject = json_object(stepResultsValue);

    workflowValue = json_value_init_object();

    if (lastInstallResultValue == NULL || stepResultsValue == NULL || workflowValue == NULL)
    {
        Log_Error("Failed to init object for json value");
        goto done;
    }

    JSON_Status jsonStatus =
        json_object_set_value(rootObject, ADUCITF_FIELDNAME_LASTINSTALLRESULT, lastInstallResultValue);
    if (jsonStatus != JSONSuccess)
    {
        Log_Error("Could not add JSON field: %s", ADUCITF_FIELDNAME_LASTINSTALLRESULT);
        goto done;
    }

    lastInstallResultValue = NULL; // rootObject owns the value now.

    //
    // State
    //
    jsonStatus = json_object_set_number(rootObject, ADUCITF_FIELDNAME_STATE, updateState);
    if (jsonStatus != JSONSuccess)
    {
        Log_Error("Could not add JSON field: %s", ADUCITF_FIELDNAME_STATE);
        goto done;
    }

    //
    // Workflow
    //
    if (!IsNullOrEmpty(workflow_peek_id(handle)))
    {
        bool success = set_workflow_properties(
            workflowValue,
            ADUC_WorkflowData_GetCurrentAction(workflowData),
            workflow_peek_id(handle),
            workflow_peek_retryTimestamp(handle));

        if (!success)
        {
            goto done;
        }

        if (json_object_set_value(rootObject, ADUCITF_FIELDNAME_WORKFLOW, workflowValue) != JSONSuccess)
        {
            Log_Error("Could not add JSON : %s", ADUCITF_FIELDNAME_WORKFLOW);
            goto done;
        }

        workflowValue = NULL; // rootObject owns the value now.
    }

    //
    // Install Update Id
    //
    if (installedUpdateId != NULL)
    {
        jsonStatus = json_object_set_string(rootObject, ADUCITF_FIELDNAME_INSTALLEDUPDATEID, installedUpdateId);
        if (jsonStatus != JSONSuccess)
        {
            Log_Error("Could not add JSON field: %s", ADUCITF_FIELDNAME_INSTALLEDUPDATEID);
            goto done;
        }
    }

    // If reporting 'downloadStarted' or 'ADUCITF_State_DeploymentInProgress' state, we must clear previous 'stepResults' map, if exists.
    if (updateState == ADUCITF_State_DownloadStarted || updateState == ADUCITF_State_DeploymentInProgress)
    {
        if (json_object_set_null(lastInstallResultObject, ADUCITF_FIELDNAME_STEPRESULTS) != JSONSuccess)
        {
            /* Note: continue the 'download' phase if we could not clear the previous results. */
            Log_Warn("Could not clear 'stepResults' property. The property may contains previous install results.");
        }
    }
    // Otherwise, we will only report 'stepResults' property if we have one or more step.
    else if (stepsCount > 0)
    {
        jsonStatus = json_object_set_value(lastInstallResultObject, ADUCITF_FIELDNAME_STEPRESULTS, stepResultsValue);
        if (jsonStatus != JSONSuccess)
        {
            Log_Error("Could not add JSON field: %s", ADUCITF_FIELDNAME_STEPRESULTS);
            goto done;
        }

        stepResultsValue = NULL; // rootObject owns the value now.
    }

    //
    // Report both state and result
    //

    // Set top-level update state and result.
    jsonStatus = _json_object_set_update_result(
        lastInstallResultObject, rootResult.ResultCode, rootResultERCs, workflow_peek_result_details(handle));

    if (jsonStatus != JSONSuccess)
    {
        goto done;
    }

    // Report all steps result.
    if (updateState != ADUCITF_State_DownloadStarted)
    {
        stepsCount = workflow_get_children_count(handle);
        for (size_t i = 0; i < stepsCount; i++)
        {
            ADUC_WorkflowHandle childHandle = workflow_get_child(handle, i);
            ADUC_Result childResult;
            JSON_Value* childResultValue = NULL;
            JSON_Object* childResultObject = NULL;
            STRING_HANDLE childUpdateId = NULL;
            STRING_HANDLE childExtendedResultCodes = NULL;

            if (childHandle == NULL)
            {
                Log_Error("Could not get components #%d update result", i);
                continue;
            }

            childResult = workflow_get_result(childHandle);

            childResultValue = json_value_init_object();
            childResultObject = json_object(childResultValue);
            if (childResultValue == NULL)
            {
                Log_Error("Could not create components update result #%d", i);
                goto childDone;
            }

            // Note: IoTHub twin doesn't support some special characters in a map key (e.g. ':', '-').
            // Let's name the result using "step_" +  the array index.
            childUpdateId = STRING_construct_sprintf("step_%d", i);
            if (childUpdateId == NULL)
            {
                Log_Error("Could not create proper child update id result key.");
                goto childDone;
            }

            jsonStatus = json_object_set_value(stepResultsObject, STRING_c_str(childUpdateId), childResultValue);
            if (jsonStatus != JSONSuccess)
            {
                Log_Error("Could not add step #%d update result", i);
                goto childDone;
            }
            childResultValue = NULL; // stepResultsValue owns it now.

            childExtendedResultCodes =
                ADUC_ReportingUtils_CreateReportingErcHexStr(childResult.ExtendedResultCode, true /* is_first */);
            jsonStatus = _json_object_set_update_result(
                childResultObject,
                childResult.ResultCode,
                childExtendedResultCodes,
                workflow_peek_result_details(childHandle));

            if (jsonStatus != JSONSuccess)
            {
                goto childDone;
            }

        childDone:
            STRING_delete(childUpdateId);
            STRING_delete(childExtendedResultCodes);
            childUpdateId = NULL;
            json_value_free(childResultValue);
            childResultValue = NULL;
        }
    }

    resultValue = rootValue;
    rootValue = NULL;

done:
    json_value_free(rootValue);
    json_value_free(lastInstallResultValue);
    json_value_free(stepResultsValue);
    json_value_free(workflowValue);
    STRING_delete(rootResultERCs);

    return resultValue;
}

/**
 * @brief Report state, and optionally result to service.
 *
 * @param workflowDataToken A workflow data object.
 * @param updateState state to report.
 * @param result Result to report (optional, can be NULL).
 * @param installedUpdateId Installed update id (if update completed successfully).
 * @return true if succeeded.
 */
bool AzureDeviceUpdateCoreInterface_ReportStateAndResultAsync(
    ADUC_WorkflowDataToken workflowDataToken,
    ADUCITF_State updateState,
    const ADUC_Result* result,
    const char* installedUpdateId)
{
    bool success = false;
    ADUC_WorkflowData* workflowData = (ADUC_WorkflowData*)workflowDataToken;

    JSON_Value* rootValue = NULL;
    char* jsonString = NULL;

    if (g_iotHubClientHandleForADUComponent == NULL)
    {
        Log_Error("ReportStateAsync called before registration! Can't report!");
        return false;
    }

    if (AgentOrchestration_ShouldNotReportToCloud(updateState))
    {
        Log_Debug("Skipping report of state '%s'", ADUCITF_StateToString(updateState));
        return true;
    }

    if (result == NULL && updateState == ADUCITF_State_DeploymentInProgress)
    {
        ADUC_Result resultForSet = { ADUC_Result_DeploymentInProgress_Success };
        workflow_set_result(workflowData->WorkflowHandle, resultForSet);
    }

    rootValue = GetReportingJsonValue(workflowData, updateState, result, installedUpdateId);
    if (rootValue == NULL)
    {
        Log_Error("Failed to get reporting json value");
        goto done;
    }

    jsonString = json_serialize_to_string(rootValue);
    if (jsonString == NULL)
    {
        Log_Error("Serializing JSON to string failed");
        goto done;
    }

    if (!ReportClientJsonProperty(ADUC_D2C_Message_Type_Device_Update_Result, jsonString, workflowData))
    {
        goto done;
    }

    success = true;

done:
    json_value_free(rootValue);
    json_free_serialized_string(jsonString);
    // Don't free the persistenceData as that will be done by the startup logic that owns it.

    return success;
}
