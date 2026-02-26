/**
 * @file test_dh_plugin_throwing.cpp
 * @brief Test plugin whose exports throw C++ exceptions.
 *
 * Has Initialize and Cleanup (so construction/destruction succeed), but
 * ProcessUpdate, OnUpdateWorkflowCompleted, and GetContractInfo throw
 * based on a configurable throw mode:
 *   0 = no throw (success)
 *   1 = throw std::runtime_error (caught by std::exception catch)
 *   2 = throw int (caught by ... catch)
 *
 * Also exports CleanupThrowing which throws (for destructor catch tests),
 * but note: the default Cleanup is non-throwing so the destructor can
 * succeed normally. The destructor exception paths are tested via the
 * minimal plugin (missing Cleanup).
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <aduc/contract_utils.h>
#include <aduc/result.h>
#include <aduc/types/update_content.h>
#include <stdexcept>

/* Avoid adu_core.h transitive dependency on logging.h */
#define TEST_ADUC_Result_Download_Handler_SuccessSkipDownload 520

typedef void* ADUC_WorkflowHandle;

static int s_throwMode = 0;

extern "C" {

void Initialize(int logLevel)
{
    (void)logLevel;
}

void Cleanup(void)
{
    /* Normal cleanup — non-throwing */
}

ADUC_Result ProcessUpdate(
    const ADUC_WorkflowHandle workflowHandle,
    const ADUC_FileEntity* fileEntity,
    const char* targetFilePath)
{
    (void)workflowHandle;
    (void)fileEntity;
    (void)targetFilePath;

    if (s_throwMode == 1)
    {
        throw std::runtime_error("test std exception from ProcessUpdate");
    }
    if (s_throwMode == 2)
    {
        throw 42;
    }

    ADUC_Result r;
    r.ResultCode = TEST_ADUC_Result_Download_Handler_SuccessSkipDownload;
    r.ExtendedResultCode = 0;
    return r;
}

ADUC_Result OnUpdateWorkflowCompleted(const ADUC_WorkflowHandle workflowHandle)
{
    (void)workflowHandle;

    if (s_throwMode == 1)
    {
        throw std::runtime_error("test std exception from OnUpdateWorkflowCompleted");
    }
    if (s_throwMode == 2)
    {
        throw 42;
    }

    ADUC_Result r;
    r.ResultCode = ADUC_GeneralResult_Success;
    r.ExtendedResultCode = 0;
    return r;
}

ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo)
{
    if (s_throwMode == 1)
    {
        throw std::runtime_error("test std exception from GetContractInfo");
    }
    if (s_throwMode == 2)
    {
        throw 42;
    }

    if (contractInfo != 0)
    {
        contractInfo->majorVer = ADUC_V1_CONTRACT_MAJOR_VER;
        contractInfo->minorVer = ADUC_V1_CONTRACT_MINOR_VER;
    }
    ADUC_Result r;
    r.ResultCode = ADUC_GeneralResult_Success;
    r.ExtendedResultCode = 0;
    return r;
}

void SetShouldFail(int mode)
{
    s_throwMode = mode;
}

} /* extern "C" */
