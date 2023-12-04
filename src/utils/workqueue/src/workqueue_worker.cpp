#include "aduc/workqueue_worker.h"

#include <aduc/workqueue.h>

#include <thread>
#include <atomic>
#include <chrono>

/**
 * @brief Worker thread function for processing work items.
 *
 * This function is responsible for continuously processing work items from a
 * given work queue using the provided work item processor callback until an
 * exit flag is set.
 *
 * @param queue The work queue to process work items from.
 * @param exitFlag An atomic boolean flag to signal the thread to exit.
 * @param processorFn The callback function for processing work items.
 */
static void WorkerThreadFn(WorkQueueHandle queue, std::atomic<bool>& exitFlag, WorkQueueItemProcessor processorFn)
{
    while (!exitFlag.load(std::memory_order_relaxed))
    {
        WorkQueueItemHandle workItem = WorkQueue_GetNextWork(queue);

        if (workItem != nullptr)
        {
            processorFn(workItem);

            WorkQueueItem_Free(workItem);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

/**
 * @brief Entry point for starting the worker thread.
 *
 * @param queue The work queue to process work items from.
 * @param processor The callback function for processing work items.
 */
extern "C" void StartWorkerThread(WorkQueueHandle queue, WorkQueueItemProcessor processor) {
    // Create an atomic exit flag
    std::atomic<bool> exitFlag(false);

    // Create a worker thread and pass the queue, exit flag, and processor callback
    std::thread worker([&]() {
        WorkerThreadFn(queue, exitFlag, processor);
    });

    // Detach the worker thread (it will clean up itself)
    worker.detach();
}

/**
 * @brief Function to signal the worker thread to exit.
 */
extern "C" void StopWorkerThread() {
    // Implement any necessary logic to signal the worker thread to exit
}
