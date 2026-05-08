/**
 * @file child_reporter.c
 * @brief High-level convenience wrapper for child process IPC reporting.
 */

#include "aduc/child_reporter.h"

#include <stdlib.h>
#include <string.h>

struct ADUC_ChildReporter
{
    ADUC_ProcessContextData context;
    ADUC_IpcClientHandle ipcClient;
};

ADUC_Result2 ADUC_ChildReporter_Init(const char* contextFilePath, ADUC_ChildReporterHandle* outHandle)
{
    if (contextFilePath == NULL || outHandle == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 1);
    }

    *outHandle = NULL;

    struct ADUC_ChildReporter* reporter = (struct ADUC_ChildReporter*)calloc(1, sizeof(struct ADUC_ChildReporter));
    if (reporter == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_RESOURCE, 1);
    }

    ADUC_Result2 result = ADUC_ProcessContext_Read(contextFilePath, &reporter->context);
    if (ADUC_RESULT2_IS_FAILURE(result))
    {
        free(reporter);
        return result;
    }

    if (reporter->context.ipcSocketPath == NULL || reporter->context.ipcSocketPath[0] == '\0')
    {
        ADUC_ProcessContext_Free(&reporter->context);
        free(reporter);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_CONFIG, 1);
    }

    result = ADUC_IpcClient_Connect(reporter->context.ipcSocketPath, &reporter->ipcClient);
    if (ADUC_RESULT2_IS_FAILURE(result))
    {
        ADUC_ProcessContext_Free(&reporter->context);
        free(reporter);
        return result;
    }

    *outHandle = reporter;
    return ADUC_RESULT2_SUCCESS;
}

const ADUC_ProcessContextData* ADUC_ChildReporter_GetContext(ADUC_ChildReporterHandle handle)
{
    if (handle == NULL)
    {
        return NULL;
    }
    return &handle->context;
}

ADUC_Result2 ADUC_ChildReporter_ReportProgress(ADUC_ChildReporterHandle handle, uint32_t percent, const char* message)
{
    if (handle == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 1);
    }

    ADUC_IpcProgressMsg msg;
    memset(&msg, 0, sizeof(msg));

    if (handle->context.stepId != NULL)
    {
        strncpy(msg.stepId, handle->context.stepId, sizeof(msg.stepId) - 1);
    }
    msg.percentComplete = percent;

    if (message != NULL)
    {
        strncpy(msg.statusMessage, message, sizeof(msg.statusMessage) - 1);
    }

    return ADUC_IpcClient_ReportProgress(handle->ipcClient, &msg);
}

ADUC_Result2 ADUC_ChildReporter_ReportSuccess(ADUC_ChildReporterHandle handle)
{
    if (handle == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 1);
    }

    ADUC_IpcResultMsg msg;
    memset(&msg, 0, sizeof(msg));

    if (handle->context.stepId != NULL)
    {
        strncpy(msg.stepId, handle->context.stepId, sizeof(msg.stepId) - 1);
    }
    msg.result.resultCode = ADUC_RESULT2_SUCCESS;
    msg.result.signal = ADUC_SIGNAL_CONTINUE;

    return ADUC_IpcClient_ReportResult(handle->ipcClient, &msg);
}

ADUC_Result2 ADUC_ChildReporter_ReportFailure(ADUC_ChildReporterHandle handle, ADUC_Result2 errorCode, const char* message)
{
    if (handle == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 1);
    }

    ADUC_IpcResultMsg msg;
    memset(&msg, 0, sizeof(msg));

    if (handle->context.stepId != NULL)
    {
        strncpy(msg.stepId, handle->context.stepId, sizeof(msg.stepId) - 1);
    }
    msg.result.resultCode = errorCode;
    msg.result.signal = ADUC_SIGNAL_ABORT_DEPLOYMENT;
    msg.result.resultDetails = message;

    return ADUC_IpcClient_ReportResult(handle->ipcClient, &msg);
}

bool ADUC_ChildReporter_IsCancelled(ADUC_ChildReporterHandle handle)
{
    if (handle == NULL)
    {
        return false;
    }
    return ADUC_IpcClient_IsCancelRequested(handle->ipcClient);
}

void ADUC_ChildReporter_Destroy(ADUC_ChildReporterHandle handle)
{
    if (handle == NULL)
    {
        return;
    }

    ADUC_IpcClient_Disconnect(handle->ipcClient);
    ADUC_ProcessContext_Free(&handle->context);
    free(handle);
}
