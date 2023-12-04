#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>
using Catch::Matchers::Equals;

#include <aduc/calloc_wrapper.hpp>
#include <aduc/workqueue.h>

using ADUC::StringUtils::cstr_wrapper;

TEST_CASE("WorkQueue API Tests", "[workqueue]")
{
    WorkQueueHandle queue_handle = WorkQueue_Create();

    SECTION("Create and Destroy Queue")
    {
        WorkQueueHandle h = WorkQueue_Create();
        CHECK(h != nullptr);
        WorkQueue_Destroy(h);
    }

    SECTION("Enqueue with invalid handle")
    {
        CHECK_FALSE(WorkQueue_EnqueueWork(nullptr, "{}"));
    }

    SECTION("Enqueue and Get Work")
    {
        REQUIRE(WorkQueue_EnqueueWork(queue_handle, "{}"));

        WorkQueueItemHandle item = WorkQueue_GetNextWork(queue_handle);
        REQUIRE(item != nullptr);

        CHECK(WorkQueueItem_GetTimeAdded(item) > 0);

        ADUC::StringUtils::cstr_wrapper wrapper{ WorkQueueItem_GetJsonPayload(item) };
        CHECK_THAT(wrapper.get(), Equals("{}"));
    }

    WorkQueue_Destroy(queue_handle);
}
