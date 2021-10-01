/**
 * @file adu_core_export_helpers.h
 * @brief Provides set of helpers for creating objects defined in adu_core_exports.h
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#ifndef ADUC_ADU_CORE_EXPORT_HELPERS_H
#define ADUC_ADU_CORE_EXPORT_HELPERS_H

#include "aduc/adu_core_exports.h"
#include "aduc/types/workflow.h"
#include "aduc/c_utils.h"
#include "aduc/types/hash.h" // for ADUC_Hash
#include "aduc/types/update_content.h" // for ADUC_FileEntity

#include <stdbool.h>

EXTERN_C_BEGIN
//
// Register/Unregister methods.
//

/**
 * @brief Call Unregister method to indicate startup.
 *
 * @param updateActionCallbacks UpdateActionCallbacks object.
 * @param argc Count of arguments in @p argv
 * @param argv Command line parameters.
 */
ADUC_Result
ADUC_MethodCall_Register(ADUC_UpdateActionCallbacks* updateActionCallbacks, unsigned int argc, const char** argv);

/**
 * @brief Call Unregister method to indicate shutdown.
 *
 * @param updateActionCallbacks UpdateActionCallbacks object.
 */
void ADUC_MethodCall_Unregister(const ADUC_UpdateActionCallbacks* updateActionCallbacks);

//
// ADU Core Interface update action methods.
//

typedef struct tagADUC_MethodCall_Data
{
    ADUC_WorkCompletionData WorkCompletionData;
    ADUC_WorkflowData* WorkflowData;
} ADUC_MethodCall_Data;

void ADUC_MethodCall_Idle(ADUC_WorkflowData* workflowData);

ADUC_Result ADUC_MethodCall_ProcessDeployment(ADUC_MethodCall_Data* methodCallData);
void ADUC_MethodCall_ProcessDeployment_Complete(ADUC_MethodCall_Data* methodCallData, ADUC_Result result);

ADUC_Result ADUC_MethodCall_Download(ADUC_MethodCall_Data* methodCallData);
void ADUC_MethodCall_Download_Complete(ADUC_MethodCall_Data* methodCallData, ADUC_Result result);

ADUC_Result ADUC_MethodCall_Install(ADUC_MethodCall_Data* methodCallData);
void ADUC_MethodCall_Install_Complete(ADUC_MethodCall_Data* methodCallData, ADUC_Result result);

ADUC_Result ADUC_MethodCall_Apply(ADUC_MethodCall_Data* methodCallData);
void ADUC_MethodCall_Apply_Complete(ADUC_MethodCall_Data* methodCallData, ADUC_Result result);

void ADUC_MethodCall_Cancel(const ADUC_WorkflowData* workflowData);

ADUC_Result ADUC_MethodCall_IsInstalled(const ADUC_WorkflowData* workflowData);

//
// State transition
//

void ADUC_SetUpdateState(ADUC_WorkflowData* workflowData, ADUCITF_State updateState);
void ADUC_SetUpdateStateWithResult(ADUC_WorkflowData* workflowData, ADUCITF_State updateState, ADUC_Result result);
void ADUC_SetInstalledUpdateIdAndGoToIdle(ADUC_WorkflowData* workflowData, const char* updateId);

//
// Reboot system
//

int ADUC_MethodCall_RebootSystem();

//
// Restart the ADU Agent.
//

int ADUC_MethodCall_RestartAgent();

EXTERN_C_END

#endif // ADUC_ADU_CORE_EXPORT_HELPERS_H
