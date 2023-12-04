#ifndef REPORTING_WORKQUEUE_WORKER_H
#define REPORTING_WORKQUEUE_WORKER_H

#include <aduc/c_utils.h>
#include <aduc/workqueue_item.h>

EXTERN_C_BEGIN

void ReportingWorkQueueItemProcessor(WorkQueueItemHandle handle);

EXTERN_C_END

#endif //REPORTING_WORKQUEUE_WORKER_H
