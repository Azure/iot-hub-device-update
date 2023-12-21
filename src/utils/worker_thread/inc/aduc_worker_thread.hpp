#pragma once

#include <atomic>
#include <functional>
#include <thread>

using WorkQueueHandle = void*;

namespace ADUC
{

using TShouldStopPredicate = std::function<bool()>;
using TWorkerFunc = std::function<void(WorkQueueHandle workQueueHandle, TShouldStopPredicate shouldStop)>;

class WorkerThread
{
public:
    WorkerThread(TWorkerFunc&& threadFunc, WorkQueueHandle workQueueHandle)
        : m_thread_fn{ std::forward<TWorkerFunc>(threadFunc) }
        , m_workQueueHandle{ workQueueHandle }
        , m_was_started{ false }
        , m_was_stopped{ false }
    {
    }

    void Start();
    void Stop();

private:
    TWorkerFunc m_thread_fn;
    WorkQueueHandle m_workQueueHandle;
    std::atomic<bool> m_stop_flag;
    bool m_was_started;
    bool m_was_stopped;
};

} // namespace ADUC
