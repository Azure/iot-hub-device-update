/**
 * @file adu_core_export_helpers.c
 * @brief Provides set of helpers for creating objects defined in adu_core_exports.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/adu_core_export_helpers.h"
#include "aduc/adu_core_interface.h"
#include "aduc/c_utils.h"
#include "aduc/extension_manager.h"
#include "aduc/hash_utils.h"
#include "aduc/logging.h"
#include "aduc/parser_utils.h"
#include "aduc/string_c_utils.h"
#include "aduc/agent_workflow.h"
#include "aduc/workflow_data_utils.h"
#include "aduc/workflow_utils.h"

#include <parson.h>
#include <stdbool.h>
#include <stdlib.h>

#include <sys/wait.h> // for waitpid
#include <unistd.h>

//
// ADUC_UpdateActionCallbacks helpers.
//

/**
 * @brief Check to see if a ADUC_UpdateActionCallbacks object is valid.
 *
 * @param updateActionCallbacks Object to verify.
 * @return _Bool True if valid.
 */
static _Bool ADUC_UpdateActionCallbacks_VerifyData(const ADUC_UpdateActionCallbacks* updateActionCallbacks)
{
    // Note: Okay for updateActionCallbacks->PlatformLayerHandle to be NULL.

    if (updateActionCallbacks->IdleCallback == NULL
        || updateActionCallbacks->DownloadCallback == NULL
        || updateActionCallbacks->InstallCallback == NULL
        || updateActionCallbacks->ApplyCallback == NULL
        || updateActionCallbacks->SandboxCreateCallback == NULL
        || updateActionCallbacks->SandboxDestroyCallback == NULL
        || updateActionCallbacks->DoWorkCallback == NULL
        || updateActionCallbacks->IsInstalledCallback == NULL)
    {
        Log_Error("Invalid ADUC_UpdateActionCallbacks object");
        return false;
    }

    return true;
}

//
// Register/Unregister methods.
//

/**
 * @brief Call into upper layer ADUC_RegisterPlatformLayer() method.
 *
 * @param updateActionCallbacks Metadata for call.
 * @param argc Count of arguments in @p argv
 * @param argv Arguments to pass to ADUC_RegisterPlatformLayer().
 * @return ADUC_Result Result code.
 */
ADUC_Result
ADUC_MethodCall_Register(ADUC_UpdateActionCallbacks* updateActionCallbacks, unsigned int argc, const char** argv)
{
    Log_Info("Calling ADUC_RegisterPlatformLayer");

    ADUC_Result result = ADUC_RegisterPlatformLayer(updateActionCallbacks, argc, argv);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        return result;
    }

    if (!ADUC_UpdateActionCallbacks_VerifyData(updateActionCallbacks))
    {
        Log_Error("Invalid ADUC_UpdateActionCallbacks structure");

        result.ResultCode = ADUC_Result_Failure;
        result.ExtendedResultCode = ADUC_ERC_NOTRECOVERABLE;
        return result;
    }

    result.ResultCode = ADUC_Result_Register_Success;
    return result;
}

/**
 * @brief Call into upper layer ADUC_Unregister() method.
 *
 * @param updateActionCallbacks Metadata for call.
 */
void ADUC_MethodCall_Unregister(const ADUC_UpdateActionCallbacks* updateActionCallbacks)
{
    Log_Info("Calling ADUC_Unregister");

    ADUC_Unregister(updateActionCallbacks->PlatformLayerHandle);
}

/**
 * @brief Call into upper layer ADUC_RebootSystem() method.
 *
 * @param workflowData Metadata for call.
 *
 * @returns int errno, 0 if success.
 */
int ADUC_MethodCall_RebootSystem()
{
    Log_Info("Calling ADUC_RebootSystem");

    return ADUC_RebootSystem();
}

/**
 * @brief Call into upper layer ADUC_RestartAgent() method.
 *
 * @param workflowData Metadata for call.
 *
 * @returns int errno, 0 if success.
 */
int ADUC_MethodCall_RestartAgent()
{
    Log_Info("Calling ADUC_RestartAgent");

    return ADUC_RestartAgent();
}
