#ifndef WORK_QUEUE_HPP
#define WORK_QUEUE_HPP

#include <ctime>
#include <memory>
#include <string>

#include <condition_variable>
#include <mutex>
#include <queue>

struct WorkQueueItem
{
    std::string json;
    std::time_t time_added;
};

class WorkQueue
{
public:
    WorkQueue(const std::string& name);
    ~WorkQueue();

    void EnqueueWork(const std::string& json);
    WorkQueueItem* GetNextWorkQueueItem();

    // noncopyable and nonmovable
    WorkQueue(const WorkQueue&) = delete;
    WorkQueue& operator=(const WorkQueue&) = delete;
    WorkQueue(WorkQueue&&) = delete;
    WorkQueue& operator=(WorkQueue&&) = delete;

private:
    std::queue<WorkQueueItem> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_workAvailable;
    std::string m_name;
};

#endif // WORK_QUEUE_HPP
