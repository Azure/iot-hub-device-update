
#include "agent_worker_threads.h"

#include <aduc_worker_thread.hpp>

#include <memory>

// forward declarations
void UpdateWorkerThreadProc(WorkQueueHandle workQueueHandle, ADUC::TShouldStopPredicate shouldStop);
void ReportingWorkerThreadProc(WorkQueueHandle workQueueHandle, ADUC::TShouldStopPredicate shouldStop);

struct AgentWorkerThreads
{
    std::unique_ptr<ADUC::WorkerThread> updateWorkerThread;
    std::unique_ptr<ADUC::WorkerThread> reportingWorkerThread;
};

WorkerThreadsHandle StartAgentWorkerThreads(ADUC_WorkQueues* workQueues)
{
    AgentWorkerThreads* workerThreads = new AgentWorkerThreads;
    workerThreads->updateWorkerThread = std::make_unique<ADUC::WorkerThread>(UpdateWorkerThreadProc, &workQueues->updateWorkQueue);
    workerThreads->reportingWorkerThread = std::make_unique<ADUC::WorkerThread>(ReportingWorkerThreadProc, &workQueues->reportingWorkQueue);
}

void StopWorkerThreads(WorkerThreadsHandle handle)
{
    auto workerThreads = reinterpret_cast<AgentWorkerThreads*>(handle);
    delete workerThreads;
}
