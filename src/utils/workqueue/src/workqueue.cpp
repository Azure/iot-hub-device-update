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

void WorkQueue::EnqueueWork(const std::string& json, void* context)
{
    std::lock_guard<std::mutex> lock(pImpl->mutex);
    pImpl->queue.emplace(WorkQueueItem{ json, std::time(nullptr), context });
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

size_t WorkQueue::GetSize() const
{
    std::lock_guard<std::mutex> lock(pImpl->mutex);
    return pImpl->queue.size();
}

/////////////////////
// BEGIN C API impl

// BEGIN WorkQueue API
// Note: All C-facing WorkQueue_* API below must not throw.

WorkQueueHandle WorkQueue_Create()
{
    try
    {
        std::unique_ptr<WorkQueue> queue = std::make_unique<WorkQueue>();
        return reinterpret_cast<WorkQueueHandle>(queue.release());
    }
    catch (...)
    {
    }
    return nullptr;
}

void WorkQueue_Destroy(WorkQueueHandle handle)
{
    try
    {
        WorkQueue* queue = reinterpret_cast<WorkQueue*>(handle);
        delete queue;
    }
    catch (...)
    {
    }
}

bool WorkQueue_EnqueueWork(WorkQueueHandle handle, const char* json, void* context)
{
    try
    {
        WorkQueue* queue = reinterpret_cast<WorkQueue*>(handle);
        if (queue == nullptr)
        {
            return false;
        }

        queue->EnqueueWork(std::string{ json }, context);
    }
    catch (...)
    {
        return false;
    }

    return true;
}

WorkQueueItemHandle WorkQueue_GetNextWork(WorkQueueHandle handle)
{
    try
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
    catch(const std::exception& e)
    {
    }
    catch(...)
    {
    }

    return nullptr;
}

bool WorkQueue_GetSize(WorkQueueHandle handle, size_t& outSize)
{
    try
    {
        WorkQueue* queue = reinterpret_cast<WorkQueue*>(handle);
        outSize = queue->GetSize();
        return true;
    }
    catch(const std::exception& e)
    {
    }
    catch(...)
    {
    }

    return false;
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

char* WorkQueueItem_GetUpdateResultMessageJson(WorkQueueItemHandle handle)
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

void* WorkQueueItem_GetContext(WorkQueueItemHandle handle)
{
    if (handle == nullptr)
    {
        return nullptr;
    }

    const WorkQueue::WorkQueueItem* item = reinterpret_cast<WorkQueue::WorkQueueItem*>(handle);
    return item->context;
}


// END WorkQueueItem API

// END C API impl
/////////////////////
