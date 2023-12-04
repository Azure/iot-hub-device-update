#include "workqueue.hpp"

#include <aduc/string_c_utils.h>
#include <aduc/workqueue.h>

#include <chrono>
#include <cstring>
#include <mutex>
#include <queue>

struct WorkQueue::Impl
{
    std::queue<WorkQueueItem> queue;
    std::mutex mutex;
};

WorkQueue::WorkQueue() : pImpl(std::make_unique<Impl>())
{
}
WorkQueue::~WorkQueue() = default;

void WorkQueue::EnqueueWork(const std::string& json)
{
    std::lock_guard<std::mutex> lock(pImpl->mutex);
    pImpl->queue.emplace(WorkQueueItem{ json, std::time(nullptr) });
}

std::unique_ptr<WorkQueue::WorkQueueItem> WorkQueue::GetNextWorkQueueItem()
{
    std::lock_guard<std::mutex> lock(pImpl->mutex);
    if (pImpl->queue.empty())
    {
        return nullptr;
    }

    auto workItem = std::make_unique<WorkQueueItem>(pImpl->queue.front());
    pImpl->queue.pop();
    return workItem;
}

/////////////////////
// BEGIN C API impl

// BEGIN WorkQueue API

WorkQueueHandle WorkQueue_Create()
{
    WorkQueue* queue = nullptr;

    try
    {
        queue = new WorkQueue;
    }
    catch (...)
    {
    }

    return reinterpret_cast<WorkQueueHandle>(queue);
}

void WorkQueue_Destroy(WorkQueueHandle handle)
{
    WorkQueue* queue = reinterpret_cast<WorkQueue*>(handle);

    try
    {
        delete queue;
    }
    catch (...)
    {
    }
}

bool WorkQueue_EnqueueWork(WorkQueueHandle handle, const char* json)
{
    WorkQueue* queue = reinterpret_cast<WorkQueue*>(handle);

    if (queue == nullptr)
    {
        return false;
    }

    try
    {
        queue->EnqueueWork(std::string{ json });
    }
    catch (...)
    {
        return false;
    }

    return true;
}

WorkQueueItemHandle WorkQueue_GetNextWork(WorkQueueHandle handle)
{
    WorkQueue* queue = reinterpret_cast<WorkQueue*>(handle);

    if (queue == nullptr)
    {
        return nullptr;
    }

    std::unique_ptr<WorkQueue::WorkQueueItem> item = queue->GetNextWorkQueueItem();
    if (item == nullptr)
    {
        return nullptr; // No work available
    }

    return item.release();
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

    const WorkQueue::WorkQueueItem* item = reinterpret_cast<WorkQueue::WorkQueueItem*>(handle);
    return item->time_added;
}

char* WorkQueueItem_GetJsonPayload(WorkQueueItemHandle handle)
{
    char* json_str = nullptr;

    if (handle == nullptr)
    {
        return nullptr;
    }

    const WorkQueue::WorkQueueItem* item = reinterpret_cast<WorkQueue::WorkQueueItem*>(handle);
    if (ADUC_AllocAndStrCopyN(&json_str, item->json.c_str(), item->json.length()) != 0)
    {
        return nullptr;
    }

    return json_str;
}

// END WorkQueueItem API

// END C API impl
/////////////////////
