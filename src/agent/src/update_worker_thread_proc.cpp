#include <aduc_worker_thread.hpp>

#include <aduc/logging.h>
#include <aduc/workqueue.h>
#include <aduc/adu_processupdate.h>
#include <aduc/agent_state_store.h>
#include <aduc/types/workflow.h>
#include <aduc/retry_utils.h>
#include <aduc/calloc_wrapper.hpp>
#include <aduc/adu_upd_utils.h> // UpdateDataFromRetriableOperationContext

#define DEBUG_WORKQUEUE 1

void UpdateWorkerThreadProc(WorkQueueHandle workQueueHandle, ADUC::TShouldStopPredicate shouldStop)
{
    ADUC_Logging_Init(ADUC_LOG_DEBUG, "update_worker");

    auto operationContext = reinterpret_cast<ADUC_Retriable_Operation_Context*>(ADUC_StateStore_GetUpdateOperationContext());

    while (!shouldStop())
    {
        try
        {
            ADUC_WorkflowHandle workqueueHandle = ADUC_StateStore_GetUpdateWorkQueueHandle();
            if (workqueueHandle == nullptr)
            {
                Log_Error("unexpected NULL workqueueHandle");
                continue;
            }

            // The call to get the next work may wait with a timeout for condition variable to notify in case
            // the queue is empty.
            WorkQueueItemHandle workItemHandle = WorkQueue_GetNextWork(workQueueHandle);

            if (workItemHandle == nullptr)
            {
                Log_Error("NULL workItemHandle");
                continue;
            }

            if (workItemHandle == -1)
            {
#ifdef DEBUG_WORKQUEUE
                Log_Debug("Get work from workqueue timed-out");
#endif // DEBUG_WORKQUEUE
                continue;
            }

            ADUC_Retriable_Operation_Context* retriableOperationContext = reinterpret_cast<ADUC_Retriable_Operation_Context*>(WorkQueueItem_GetContext(workItemHandle));
            if (retriableOperationContext == nullptr)
            {
                continue;
            }

            ADUC_Update_Request_Operation_Data* updateData = UpdateDataFromRetriableOperationContext(retriableOperationContext);
            ADUC::StringUtils::cstr_wrapper json{ WorkQueueItem_GetUpdateResultMessageJson(workItemHandle) };

            // This call will block until either processing of update is complete, or it exits early due to it
            // periodically checking that the workqueue is not empty and finding a valid retry, replace, or cancel
            // request (if the workflowId is the same then it will be ignored and current processing will continue
            // without early exit).
            bool was_not_early_exit = Adu_ProcessUpdate(std::string{ json.get() }, updateData, retriableOperationContext);
            if (!was_not_early_exit)
            {
                Log_Info("Exited workflow processing early due to incoming cancel or retry/replacement workflow");
            }

        }
        catch(const std::exception& e)
        {
            Log.Error("std::exception: %s", e.what());
        }
        catch(...)
        {
            Log.Error("unknown exception: %s");
        }
    }

    ADUC_Logging_Uninit();
}
