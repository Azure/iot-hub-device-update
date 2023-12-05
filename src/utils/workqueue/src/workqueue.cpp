#include "workqueue.hpp"

#include <aduc/logging.h>
#include <aduc/string_c_utils.h>
#include <aduc/workqueue.h>

#include <cassert>
#include <chrono>
#include <cstring>

using namespace std::chrono_literals;

WorkQueue::WorkQueue(const std::string& name) : m_name{ name }
{
}

WorkQueue::~WorkQueue() = default;

void WorkQueue::EnqueueWork(const std::string& json)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_queue.emplace(WorkQueueItem{ json, std::time(nullptr) });

    m_workAvailable.notify_one();
}

// TODO: allow passing in a timeout
WorkQueueItem* WorkQueue::GetNextWorkQueueItem()
{
    std::unique_lock<std::mutex> mutLock(m_mutex);
    if (m_workAvailable.wait_until(mutLock, std::chrono::system_clock::now() + 15000ms, [this]() { return !m_queue.empty(); }))
    {
        Log_Debug("got work on work queue '%s'", m_name.c_str());

        std::unique_ptr<WorkQueueItem> workItem = std::make_unique<WorkQueueItem>(m_queue.front());
        m_queue.pop();
        return workItem.release();
    }
    else
    {
        Log_Debug("timed-out waiting for work on work queue '%s'", m_name.c_str());
        return nullptr;
    }
}

/////////////////////
// BEGIN C API impl

// BEGIN WorkQueue API

WorkQueueHandle WorkQueue_Create(const char* name)
{
    WorkQueue* work_queue = nullptr;

    try
    {
        work_queue = new WorkQueue{ std::string{ name } };
    }
    catch (...)
    {
    }

    return reinterpret_cast<WorkQueueHandle>(work_queue);
}

void WorkQueue_Destroy(WorkQueueHandle handle)
{
    WorkQueue* work_queue = reinterpret_cast<WorkQueue*>(handle);

    try
    {
        delete work_queue;
    }
    catch (...)
    {
    }
}

bool WorkQueue_EnqueueWork(WorkQueueHandle handle, const char* json)
{
    WorkQueue* work_queue = reinterpret_cast<WorkQueue*>(handle);

    if (work_queue == nullptr)
    {
        return false;
    }

    try
    {
        work_queue->EnqueueWork(std::string{ json });
    }
    catch (...)
    {
        return false;
    }

    return true;
}

WorkQueueItemHandle WorkQueue_GetNextWork(WorkQueueHandle handle)
{
    WorkQueue* work_queue = reinterpret_cast<WorkQueue*>(handle);

    if (work_queue == nullptr)
    {
        return nullptr;
    }

    std::unique_ptr<WorkQueueItem> item{ work_queue->GetNextWorkQueueItem() };
    if (!item)
    {
        return nullptr; // No work available
    }

    return reinterpret_cast<WorkQueueItemHandle>(item.release());
}

// END WorkQueue API

// BEGIN WorkQueueItem API

time_t WorkQueueItem_GetTimeAdded(WorkQueueItemHandle handle)
{
    time_t time_added;
    memset(&time_added, 0, sizeof(time_added));

    if (handle == nullptr)
    {
        return time_added;
    }

    const WorkQueueItem* item = reinterpret_cast<WorkQueueItem*>(handle);
    return item->time_added;
}

char* WorkQueueItem_GetJsonPayload(WorkQueueItemHandle handle)
{
    char* json_str = nullptr;

    if (handle == nullptr)
    {
        return nullptr;
    }

    const WorkQueueItem* item = reinterpret_cast<WorkQueueItem*>(handle);
    if (ADUC_AllocAndStrCopyN(&json_str, item->json.c_str(), item->json.length()) != 0)
    {
        return nullptr;
    }

    return json_str;
}

void WorkQueueWorkItem_Free(WorkQueueItemHandle handle)
{
    WorkQueueItem* item = reinterpret_cast<WorkQueueItem*>(handle);
    delete item;
}

// END WorkQueueItem API

// END C API impl
/////////////////////
