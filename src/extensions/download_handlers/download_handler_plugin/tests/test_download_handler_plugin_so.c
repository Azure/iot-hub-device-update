/**
 * @file test_download_handler_plugin_so.c
 * @brief A minimal test download handler plugin shared library.
 *
 * Provides the 5 export symbols expected by DownloadHandlerPlugin:
 *   Initialize, Cleanup, ProcessUpdate, OnUpdateWorkflowCompleted, GetContractInfo
 *
 * A global flag controls whether ProcessUpdate/OnUpdateWorkflowCompleted return
 * success or failure, allowing tests to exercise both paths.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <aduc/contract_utils.h>
#include <aduc/result.h>
#include <aduc/types/update_content.h>

/* Value from adu_core.h enum — avoid including it to prevent transitive dependency on logging.h */
#define TEST_ADUC_Result_Download_Handler_SuccessSkipDownload 520

typedef void* ADUC_WorkflowHandle;

/* A global that tests can modify (via a separate export) to control behavior. */
static int s_shouldFail = 0;

void Initialize(int logLevel)
{
    (void)logLevel;
}

void Cleanup(void)
{
}

ADUC_Result ProcessUpdate(
    const ADUC_WorkflowHandle workflowHandle,
    const ADUC_FileEntity* fileEntity,
    const char* targetUpdateFilePath)
{
    ADUC_Result result;
    (void)workflowHandle;
    (void)fileEntity;
    (void)targetUpdateFilePath;

    if (s_shouldFail)
    {
        result.ResultCode = ADUC_GeneralResult_Failure;
        result.ExtendedResultCode = 0x12345678;
    }
    else
    {
        result.ResultCode = TEST_ADUC_Result_Download_Handler_SuccessSkipDownload;
        result.ExtendedResultCode = 0;
    }
    return result;
}

ADUC_Result OnUpdateWorkflowCompleted(const ADUC_WorkflowHandle workflowHandle)
{
    ADUC_Result result;
    (void)workflowHandle;

    if (s_shouldFail)
    {
        result.ResultCode = ADUC_GeneralResult_Failure;
        result.ExtendedResultCode = 0x87654321;
    }
    else
    {
        result.ResultCode = ADUC_GeneralResult_Success;
        result.ExtendedResultCode = 0;
    }
    return result;
}

ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo)
{
    ADUC_Result result;
    if (contractInfo != 0)
    {
        contractInfo->majorVer = ADUC_V1_CONTRACT_MAJOR_VER;
        contractInfo->minorVer = ADUC_V1_CONTRACT_MINOR_VER;
    }
    result.ResultCode = ADUC_GeneralResult_Success;
    result.ExtendedResultCode = 0;
    return result;
}

/* Additional export to allow tests to toggle failure mode. */
void SetShouldFail(int fail)
{
    s_shouldFail = fail;
}
