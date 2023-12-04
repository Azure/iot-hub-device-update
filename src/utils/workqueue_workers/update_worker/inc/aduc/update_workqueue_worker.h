#ifndef UPDATE_WORKQUEUE_WORKER_H
#define UPDATE_WORKQUEUE_WORKER_H

#include <aduc/c_utils.h>
#include <aduc/workqueue_item.h>

EXTERN_C_BEGIN

void UpdateWorkQueueItemProcessor(WorkQueueItemHandle handle);

EXTERN_C_END

#endif //UPDATE_WORKQUEUE_WORKER_H
