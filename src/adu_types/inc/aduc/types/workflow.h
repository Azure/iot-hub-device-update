/**
 * @file workflow.h
 * @brief Define types for Device Update agent workflow.
 *
 * @copyright Copyright (c) Microsoft Corp.
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

// TODO(Nox): 34317366: [Code Clean Up] Consolidate tagADUC_WorkflowData and tagADUC_Workflow
//            Move all ADUC_WorkflowData members into ADUC_WorkflowHandle.

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

    ADUC_UpdateActionCallbacks UpdateActionCallbacks; /**< Upper-level registration data; function pointers, etc. */

    _Bool IsRegistered; /**< True if UpdateActionCallbacks is valid and needs to be ultimately unregistered. */

    _Bool StartupIdleCallSent; /**< True once the initial Idle call is sent to the orchestrator on agent startup. */

    _Bool OperationCancelled; /**< Was the operation in progress requested to cancel? */

    ADUC_SystemRebootState SystemRebootState; /**< The system reboot state. */

    ADUC_AgentRestartState AgentRestartState; /**< The agent restart state. */

    ADUC_DownloadProgressCallback DownloadProgressCallback; /**< Callback for download progress. */

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

} ADUC_WorkflowData;

#endif // ADUC_TYPES_WORKFLOW_H
