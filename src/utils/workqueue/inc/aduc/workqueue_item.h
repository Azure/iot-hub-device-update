#ifndef UPDATE_WORKITEM_H
#define UPDATE_WORKITEM_H

#include <aduc/c_utils.h>
#include <time.h>

typedef void* WorkQueueItemHandle;

EXTERN_C_BEGIN

time_t WorkQueueItem_GetTimeAdded(WorkQueueItemHandle handle);
char* WorkQueueItem_GetData(WorkQueueItemHandle handle);

EXTERN_C_END

#endif // UPDATE_WORKITEM_H
