#ifndef WORKQUEUE_WORKER_H
#define WORKQUEUE_WORKER_H

#include <aduc/c_utils.h>

typedef void* WorkQueueHandle;
typedef void* WorkQueueItemHandle;

/**
 * @brief Func pointer for processor of work item in the queue.
 * @remark Processor functions MUST not free the WorkQueueItemHandle (there should NOT be a header function).
 * The workqueue will free the work item on behalf of the processor once processor returns.
 *
 */
typedef void (*WorkQueueItemProcessor)(WorkQueueItemHandle handle);

// Declaration of the worker thread functions
EXTERN_C_BEGIN

void StartWorkQueueWorkerThread(WorkQueueHandle queue, WorkQueueItemProcessor processor, const char* thread_name);
void StopAllWorkQueueWorkerThreads();

EXTERN_C_END

#endif // WORKQUEUE_WORKER_H
