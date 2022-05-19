/**
 * @file workflow.h
 * @brief Define types for Device Update agent workflow.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_TYPES_WORKFLOW_H
#define ADUC_TYPES_WORKFLOW_H

#include <limits.h>
#include <stdbool.h>

#include "aduc/result.h"
#include "aduc/types/adu_core.h"
#include "aduc/types/download.h"
#include "aduc/types/hash.h"
#include "aduc/types/update_content.h"

#include "parson.h"

typedef void* ADUC_WorkflowHandle;

/**
 * @brief Update Manifest data for the workflow
 */
typedef struct tagADUC_ContentData
{
    ADUC_UpdateId* ExpectedUpdateId; /**< The expected/desired update Id. Required. */
    char* InstalledCriteria; /**< The installed criteria string used to evaluate if content is installed. Required. */
    char* UpdateType; /**< The content type string. Required. */
    char* UpdateTypeName; /**< The provider/name part of the Update Type. */
    unsigned int UpdateTypeVersion; /** The version number of the Update Type. */
} ADUC_ContentData;

/**
 * @brief An Update Content Handler can set this state to indicates whether an agent restart is required.
 */
typedef enum tagADUC_AgentRestartState
{
    ADUC_AgentRestartState_None = 0, /** Agent restart not required after Apply completed. */
    ADUC_AgentRestartState_Required = 1, /** Agent restart required, but not initiated yet. */
    ADUC_AgentRestartState_InProgress = 2 /** Agent restart is in progress. */
} ADUC_AgentRestartState;

/**
 * @brief An Update Content Handler can set this state to indicates whether a device reboot is required.
 */
typedef enum tagADUC_SystemRebootState
{
    ADUC_SystemRebootState_None = 0, /** System reboot not required after Apply completed. */
    ADUC_SystemRebootState_Required = 1, /** System reboot required, but not initiated yet. */
    ADUC_SystemRebootState_InProgress = 2 /** System reboot is in progress. */
} ADUC_SystemRebootState;

/**
 * @brief The different types of workflow cancellation requests.
 */
typedef enum tagADUC_WorkflowCancellationType
{
    ADUC_WorkflowCancellationType_None             = 0, /**< No cancellation. */
    ADUC_WorkflowCancellationType_Normal           = 1, /**< A normal cancel due to a Cancel update action from the cloud. */
    ADUC_WorkflowCancellationType_Replacement      = 2, /**< A cancel due to a process deployment update action from the cloud for a workflow with a different workflow id . */
    ADUC_WorkflowCancellationType_Retry            = 3, /**< A cancel due to a process deployment update action from the cloud for the same workflow id but with a new retry timestamp token. */
    ADUC_WorkflowCancellationType_ComponentChanged = 4, /**< A cancel due to a components changed event. */
} ADUC_WorkflowCancellationType;

/**
 * @brief Function signature for callback to send download progress to.
 */
typedef void (*ADUC_Core_DownloadFunction)(
    const char* workflowId,
    const char* fileId,
    ADUC_DownloadProgressState state,
    uint64_t bytesTransferred,
    uint64_t bytesTotal);

// Forward decl.
struct tagADUC_WorkflowData;

typedef ADUC_Result (*ADUC_Core_Download_Function)(
    const ADUC_FileEntity* entity,
    const char* workflowId,
    const char* workFolder,
    ADUC_DownloadProgressCallback downloadProgressCallback);

typedef ADUC_Result (*ADUC_Set_Workflow_Result_Function)(
    const char* workflowId, ADUC_Result result, _Bool reportToCloud, _Bool persistLocally);

typedef void (*ADUC_WorkflowData_Free_Function)(struct tagADUC_WorkflowData* workflowData);

typedef void (*HandleUpdateActionFunc)(struct tagADUC_WorkflowData* workflowData);
typedef void (*SetUpdateStateWithResultFunc)(struct tagADUC_WorkflowData* workflowData, ADUCITF_State updateState, ADUC_Result result);
typedef void (*WorkCompletionCallbackFunc)(const void* workCompletionToken, ADUC_Result result, _Bool isAsync);
typedef int (*RebootSystemFunc)();
typedef int (*RestartAgentFunc)();

typedef void* ADUC_CLIENT_HANDLE_TYPE;
typedef void (*IOTHUB_CLIENT_REPORTED_STATE_CALLBACK_TYPE)(int, void*);
typedef enum ADUC_IOTHUB_CLIENT_RESULT_TAG {
    ADUC_IOTHUB_CLIENT_OK,
    ADUC_IOTHUB_CLIENT_INVALID_ARG,
    ADUC_IOTHUB_CLIENT_ERROR,
    ADUC_IOTHUB_CLIENT_INVALID_SIZE,
    ADUC_IOTHUB_CLIENT_INDEFINITE_TIME
} IOTHUB_CLIENT_RESULT_TYPE;
typedef IOTHUB_CLIENT_RESULT_TYPE (ClientHandleSendReportType)(ADUC_CLIENT_HANDLE_TYPE, const unsigned char *, size_t, IOTHUB_CLIENT_REPORTED_STATE_CALLBACK_TYPE, void *);
typedef ClientHandleSendReportType* ClientHandleSendReportFunc;

#ifdef ADUC_BUILD_UNIT_TESTS
typedef struct tagADUC_TestOverride_Hooks
{
    void* ContentHandler_TestOverride;
    HandleUpdateActionFunc HandleUpdateActionFunc_TestOverride;
    SetUpdateStateWithResultFunc SetUpdateStateWithResultFunc_TestOverride;
    WorkCompletionCallbackFunc WorkCompletionCallbackFunc_TestOverride;
    RebootSystemFunc RebootSystemFunc_TestOverride;
    RestartAgentFunc RestartAgentFunc_TestOverride;
    void* ClientHandle_SendReportedStateFunc_TestOverride;
} ADUC_TestOverride_Hooks;
#endif // #ifdef ADUC_BUILD_UNIT_TESTS

/**
 * @brief A callback function for reporting state, and optionally result to service.
 *
 * @param workflowData A workflow data object.
 * @param updateState state to report.
 * @param result Result to report (optional, can be NULL).
 * @param installedUpdateId Installed update id (if update completed successfully).
 * @return true if succeeded.
 */
typedef _Bool (*ADUC_ReportStateAndResultAsyncCallback)(
    ADUC_WorkflowDataToken workflowData,
    ADUCITF_State updateState,
    const ADUC_Result* result,
    const char* installedUpdateId);

/**
 * @brief Data shared across the agent workflow.
 */
typedef struct tagADUC_WorkflowData
{
    ADUC_WorkflowHandle WorkflowHandle;
    char* LogFolder; /** <Log files folder. */

    //
    // Update action data
    //
    ADUCITF_UpdateAction CurrentAction; /**< Value of "action" from UpdateActionJson. */

    ADUC_ContentData ContentData; /**< The update data for this workflow. */

    //
    // Workflow states
    //
    ADUC_Result Result; /**< Current workflow result data. */
    ADUCITF_State LastReportedState; /**< Last state set for the workflow and may have been reported as per agent orchestration. */
    char* LastCompletedWorkflowId; /**< Last workflow id for deployment that completed successfully. */

    ADUC_UpdateActionCallbacks UpdateActionCallbacks; /**< Upper-level registration data; function pointers, etc. */

    _Bool IsRegistered; /**< True if UpdateActionCallbacks is valid and needs to be ultimately unregistered. */

    _Bool StartupIdleCallSent; /**< True once the initial Idle call is sent to the orchestrator on agent startup. */

    _Bool OperationCancelled; /**< Was the operation in progress requested to cancel? */

    ADUC_SystemRebootState SystemRebootState; /**< The system reboot state. */

    ADUC_AgentRestartState AgentRestartState; /**< The agent restart state. */

    ADUC_DownloadProgressCallback DownloadProgressCallback; /**< Callback for download progress. */

    ADUC_ReportStateAndResultAsyncCallback ReportStateAndResultAsyncCallback; /**< Callback for reporting workflow state and result. */

    /**
     * @brief Results object
     *
     *  {
     *      "workflowId" : "root",
     *      "results" : {
     *          "root" : {
     *              "lastReportedState": 0,
     *              "currentState" : 0,
     *              "resultCode" : 0,
     *              "extendedResultCode" : 0,
     *              "resultDetails" : "",
     *              "childIds" : [ "root/0", "root/1"];
     *          },
     *          "root/0" : {
     *              "lastReportedState": 0,
     *              "currentState" : 0,
     *              "resultCode" : 0,
     *              "extendedResultCode" : 0,
     *              "resultDetails" : "",
     *              "childIds" : [ "root/0/comp0", "root/0/comp1"];
     *          },
     *          "root/0/comp0" : {
     *              "lastReportedState": 0,
     *              "currentState" : 0,
     *              "resultCode" : 0,
     *              "extendedResultCode" : 0,
     *              "resultDetails" : "",
     *              "childIds" : [];
     *          },
     *          "root/0/comp1" : {
     *              "lastReportedState": 0,
     *              "currentState" : 0,
     *              "resultCode" : 0,
     *              "extendedResultCode" : 0,
     *              "resultDetails" : "",
     *              "childIds" : [];
     *          },
     *          "root/1" : {
     *              "lastReportedState": 0,
     *              "currentState" : 0,
     *              "resultCode" : 0,
     *              "extendedResultCode" : 0,
     *              "resultDetails" : "",
     *              "childIds" : [ "root/1/comp0"];
     *          },
     *          "root/1/comp0" : {
     *              "lastReportedState": 0,
     *              "currentState" : 0,
     *              "resultCode" : 0,
     *              "extendedResultCode" : 0,
     *              "resultDetails" : "",
     *              "childIds" : [];
     *          },
     *      }
     *  }
     *
     */
    JSON_Array* Results;

    char* LastGoalStateJson; /**< The goal state data sent from DU Service to DU Agent. This data is needed when re-processing latest update on the device */

#ifdef ADUC_BUILD_UNIT_TESTS
    ADUC_TestOverride_Hooks* TestOverrides; /**< Test hook overrides. This will be NULL when not testing. */
#endif

} ADUC_WorkflowData;

#endif // ADUC_TYPES_WORKFLOW_H
