#ifndef WORK_QUEUE_HPP
#define WORK_QUEUE_HPP

#include <ctime>
#include <memory>
#include <string>

class WorkQueue
{
public:
    struct WorkQueueItem
    {
        std::string json;
        std::time_t time_added;
        void* context;
    };

    WorkQueue();
    ~WorkQueue();

    void EnqueueWork(const std::string& json, void* context);
    std::unique_ptr<WorkQueueItem> GetNextWorkQueueItem();
    size_t GetSize() const;

    // noncopyable and nonmovable
    WorkQueue(const WorkQueue&) = delete;
    WorkQueue& operator=(const WorkQueue&) = delete;
    WorkQueue(WorkQueue&&) = delete;
    WorkQueue& operator=(WorkQueue&&) = delete;

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

#endif // WORK_QUEUE_HPP
