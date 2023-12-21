#ifndef ADUC_WORKQUEUE_H
#define ADUC_WORKQUEUE_H

#include "workqueue_item.h"

#include <aduc/c_utils.h>
#include <stdbool.h>

EXTERN_C_BEGIN

typedef void* WorkQueueHandle;

typedef struct tagADUC_WorkQueues
{
    WorkQueueHandle updateWorkQueue;
    WorkQueueHandle reportingWorkQueue;
} ADUC_WorkQueues;

WorkQueueHandle WorkQueue_Create();
void WorkQueue_Destroy(WorkQueueHandle queue);
bool WorkQueue_EnqueueWork(WorkQueueHandle queue, const char* json, void* context);
WorkQueueItemHandle WorkQueue_GetNextWork(WorkQueueHandle queue);
size_t WorkQueue_GetSize(WorkQueueHandle queue);

EXTERN_C_END

#endif // ADUC_WORKQUEUE_H
