#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <aduc/workqueue.h>
#include <aduc/workqueue_worker.h>
#include <aduc/calloc_wrapper.hpp>

#include <thread>
#include <atomic>
#include <chrono>

static std::string processed_json;
time_t process_time_added;

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

TEST_CASE("WorkQueue Worker Processing Test", "[workqueue]")
{
    const std::string expected_json{ "{ \"foo\": \"bar\" }" };
    processed_json = "";
    process_time_added = 0;

    WorkQueueHandle testQueue = WorkQueue_Create();
    REQUIRE(testQueue != nullptr);

    WorkQueueHandle testQueue = WorkQueue_Create();
    StartWorkerThread(&testQueue, MockProcessor);

    WorkQueue_EnqueueWork(testQueue, expected_json.c_str());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    StopWorkerThread();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    WorkQueue_Destroy(testQueue);

    CHECK(processed_json == expected_json);
    CHECK(process_time_added != 0);
}
