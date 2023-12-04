#ifndef WORKQUEUE_WORKER_H
#define WORKQUEUE_WORKER_H

#include <aduc/c_utils.h>

typedef void* WorkQueueHandle;
typedef void* WorkQueueItemHandle;

/**
 * @brief Func pointer for processor of work item in the queue.
 *
 */
typedef void (*WorkQueueItemProcessor)(WorkQueueItemHandle handle);

// Declaration of the worker thread functions
EXTERN_C_BEGIN

void StartWorkerThread(WorkQueueHandle queue, WorkQueueItemProcessor processor);
void StopWorkerThread();

EXTERN_C_END

#endif // WORKQUEUE_WORKER_H
