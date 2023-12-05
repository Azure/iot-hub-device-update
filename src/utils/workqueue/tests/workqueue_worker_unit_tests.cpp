#include <catch2/catch.hpp>

#include <aduc/workqueue.h>
#include <aduc/workqueue_worker.h>
#include <aduc/calloc_wrapper.hpp>

#include <thread>
#include <atomic>
#include <chrono>

static std::string processed_json;
time_t process_time_added;

static std::string processed_json2;
time_t process_time_added2;

// Mocked work queue and work item
WorkQueueHandle testWorkQueueHandle;

// Mocked processor function for testing
void MockProcessor(WorkQueueItemHandle handle)
{
    char* json = WorkQueueItem_GetJsonPayload(handle);
    ADUC::StringUtils::cstr_wrapper wrapper{ json };
    processed_json = json;

    process_time_added = WorkQueueItem_GetTimeAdded(handle);
}

void MockProcessor2(WorkQueueItemHandle handle)
{
    char* json = WorkQueueItem_GetJsonPayload(handle);
    ADUC::StringUtils::cstr_wrapper wrapper{ json };
    processed_json2 = json;

    process_time_added2 = WorkQueueItem_GetTimeAdded(handle);
}

TEST_CASE("WorkQueue Worker Processing Test", "[workqueue]")
{
    const std::string expected_json{ "{ \"foo\": \"bar\" }" };
    processed_json = "";
    process_time_added = 0;

    WorkQueueHandle testQueue = WorkQueue_Create("testqueue");
    REQUIRE(testQueue != nullptr);

    StartWorkQueueWorkerThread(&testQueue, MockProcessor, "WorkQueue Worker Processing Test");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    WorkQueue_EnqueueWork(testQueue, expected_json.c_str());
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    StopAllWorkQueueWorkerThreads();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    WorkQueue_Destroy(testQueue);

    CHECK(processed_json == expected_json);
    CHECK(process_time_added != 0);
}

TEST_CASE("Multiple WorkQueues each with Worker", "[workqueue]")
{
    const std::string expected_json1{ "{ \"foo\": \"bar\" }" };
    const std::string expected_json2{ "{}" };

    processed_json = "";
    process_time_added = 0;

    processed_json2 = "";
    process_time_added2 = 0;

    WorkQueueHandle testQueue = WorkQueue_Create("workqueue1");
    REQUIRE(testQueue != nullptr);

    WorkQueueHandle testQueue2 = WorkQueue_Create("workqueue2");
    REQUIRE(testQueue2 != nullptr);

    StartWorkQueueWorkerThread(&testQueue, MockProcessor, "WorkQueue#1");
    StartWorkQueueWorkerThread(&testQueue2, MockProcessor2, "WorkQueue#2");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    WorkQueue_EnqueueWork(testQueue, expected_json1.c_str());
    WorkQueue_EnqueueWork(testQueue2, expected_json2.c_str());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    StopAllWorkQueueWorkerThreads();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    WorkQueue_Destroy(testQueue);
    WorkQueue_Destroy(testQueue2);

    CHECK(processed_json == expected_json1);
    CHECK(process_time_added != 0);

    CHECK(processed_json == expected_json2);
    CHECK(process_time_added2 != 0);
}
