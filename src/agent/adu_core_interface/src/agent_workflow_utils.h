/**
 * @file agent_workflow_utils.h
 * @brief Private header for util functions for agent_workflow.
 *
 * These are declared in a private header for code sharing and unit testing.
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#ifndef AGENT_WORKFLOW_UTILS_H
#define AGENT_WORKFLOW_UTILS_H

#include "aduc/adu_core_interface.h"
#include "aduc/agent_workflow.h"

#include <aduc/c_utils.h>

EXTERN_C_BEGIN

_Bool IsDuplicateRequest(ADUCITF_UpdateAction action, ADUCITF_State lastReportedState);

ADUC_ContentData* ADUC_ContentData_AllocAndInit(const JSON_Value* updateActionJson);
_Bool ADUC_ContentData_Update(
    ADUC_ContentData* contentData, const JSON_Value* updateActionJson, _Bool requiredAllData);
void ADUC_ContentData_Free(ADUC_ContentData* contentData);

EXTERN_C_END

#endif // AGENT_WORKFLOW_UTILS_H
