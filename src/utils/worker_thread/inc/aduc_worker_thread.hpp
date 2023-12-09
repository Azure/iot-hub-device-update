#ifndef ADU_WORKERTHREADS_HPP
#define ADU_WORKERTHREADS_HPP

#include <atomic>
#include <functional>
#include <thread>

namespace ADUC
{

using TShouldStopPredicate = std::function<bool()>;
using TWorkerFunc = std::function<void(TShouldStopPredicate shouldStop)>;

class WorkerThread
{
public:
    WorkerThread(TWorkerFunc&& threadFunc)
        : m_thread_fn{ std::forward<TWorkerFunc>(threadFunc) } // forward preserves L/R-Value-ness
        , m_was_started{ false }
        , m_was_stopped{ false }
    {
    }

    void Start();
    void Stop();

private:
    TWorkerFunc m_thread_fn;
    std::atomic<bool> m_stop_flag;
    bool m_was_started;
    bool m_was_stopped;
};

} // namespace ADUC

#endif // ADU_WORKERTHREADS_HPP
