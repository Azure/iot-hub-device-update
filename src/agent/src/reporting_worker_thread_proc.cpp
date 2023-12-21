#include <aduc_worker_thread.hpp>

#include <aduc/logging.h>
#include <aduc/workqueue.h>

void ReportingWorkerThreadProc(WorkQueueHandle workQueueHandle, ADUC::TShouldStopPredicate shouldStop)
{
    ADUC_Logging_Init(ADUC_LOG_DEBUG, "reporting_worker");

    while (!shouldStop())
    {
        WorkQueueItemHandle workItemHandle = WorkQueue_GetNextWork(workQueueHandle);
    }

    ADUC_Logging_Uninit();
}

