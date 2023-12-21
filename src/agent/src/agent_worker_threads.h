#ifndef ADUC_AGENT_WORKER_THREADS_H
#define ADUC_AGENT_WORKER_THREADS_H

#include <aduc/c_utils.h>
#include <aduc/workqueue.h>

EXTERN_C_BEGIN

typedef void* WorkerThreadsHandle;

WorkerThreadsHandle StartAgentWorkerThreads(ADUC_WorkQueues* workQueues);
void StopAgentWorkerThreads(WorkerThreadsHandle handle);

EXTERN_C_END

#endif // ADUC_AGENT_WORKER_THREADS_H
