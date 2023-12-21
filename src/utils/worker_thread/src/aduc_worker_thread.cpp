#include "aduc_worker_thread.hpp"

namespace ADUC
{

void WorkerThread::Start()
{
    if (m_was_started)
    {
        return;
    }

    m_stop_flag.store(false, std::memory_order_seq_cst);

    // We pass shouldStop lambda to the thread function so that the
    // user of ADUC::WorkerThread that provides the thread function can just
    // call shouldStop() to know if it should exit without having to know
    // about m_stop_flag details.
    // We use std::memory_order_seq_cst as it is the strongest consistency
    // guarantee and the assumption is that this is not performance critical
    // code. If that assumption changes later, then it can be changed to use
    // memory_order_release for setting and memory_order_acquire for checking.
    auto thrd = std::thread(m_thread_fn, m_workQueueHandle, [this]() -> bool
    {
        return m_stop_flag.load(std::memory_order_seq_cst);
    });

    m_was_started = true;
    thrd.detach();
}

void WorkerThread::Stop()
{
    if (m_was_stopped)
    {
        return;
    }

    m_stop_flag.store(true, std::memory_order_seq_cst);
}

} // namespace ADUC
