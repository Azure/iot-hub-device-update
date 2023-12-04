
#include "aduc/update_workqueue_worker.h"

void UpdateWorkQueueItemProcessor(WorkQueueItemHandle handle)
{
    WorkQueueItem_Free(handle);
}
