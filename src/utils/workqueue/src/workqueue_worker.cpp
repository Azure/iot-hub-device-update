#include "aduc/workqueue_worker.h"

#include <aduc/logging.h>
#include <aduc/workqueue.h>

#include <thread>
#include <atomic>
#include <chrono>

// forward declaration
void WorkQueueWorkItem_Free(WorkQueueItemHandle handle);

// single exit flag for now for all work queue workers threads to exit.
static std::atomic<bool> s_exitFlag{ false };

/**
 * @brief Worker thread function for processing work items.
 *
 * This function is responsible for continuously processing work items from a
 * given work queue using the provided work item processor callback until an
 * exit flag is set.
 *
 * @param queue The work queue to process work items from.
 * @param processorFn The callback function for processing work items.
 * @param thread_name The name to use in logging.
 */
static void WorkerThreadFn(WorkQueueHandle handle, WorkQueueItemProcessor processorFn, const char* thread_name)
{
    Log_Info("Starting worker thread '%s'", thread_name);

    while (!s_exitFlag.load(std::memory_order_relaxed))
    {
        WorkQueueItemHandle workItem = WorkQueue_GetNextWork(handle);

        // The workItem can be nullptr if it times out; otherwise, it got an item queued
        // before the timeout elapsed.
        if (workItem != nullptr)
        {
            try
            {
                Log_Debug("worker '%s' calling processor fn on item", thread_name);
                processorFn(workItem);
            }
            catch (const std::exception& ex)
            {
                Log_Error("exception while processing workitem for '%s' worker: '%s'", thread_name, ex.what());
            }
            catch (...)
            {
                Log_Error("Unknown exception processing workitem for '%s' worker", thread_name);
            }

            WorkQueueWorkItem_Free(workItem);
        }
    }

    Log_Warn("Stopping worker thread '%s'", thread_name);
}

/**
 * @brief Entry point for starting the worker thread.
 *
 * @param queue The work queue to process work items from.
 * @param processor The callback function for processing work items.
 * @param thread_name The name used in logging.
 */
void StartWorkQueueWorkerThread(WorkQueueHandle queue, WorkQueueItemProcessor processor, const char* thread_name)
{
    // Create a worker thread and pass the queue, exit flag, and processor callback
    std::thread worker([queue, processor, thread_name]() {
        WorkerThreadFn(queue, processor, thread_name);
    });

    worker.detach();
}

/**
 * @brief Function to signal the worker threads in all work queues to exit.
 */
void StopAllWorkQueueWorkerThreads()
{
    s_exitFlag.store(true, std::memory_order_relaxed);
}
