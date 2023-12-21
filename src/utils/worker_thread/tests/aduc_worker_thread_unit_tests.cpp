#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>

#include <aduc_worker_thread.hpp>
#include <aduc/workqueue.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

TEST_CASE("Worker Thread Tests", "[threads]")
{
    const unsigned MaxIterations = 1000;

    std::vector<std::string> thread_exec_pushes;

    std::unique_ptr<WorkQueueHandle, WorkQueue_Destroy> workQueueHandle{ WorkQueue_Create() };
    REQUIRE(workQueueHandle.get() != NULL);

    ADUC::WorkerThread simple_thread([&thread_exec_pushes](WorkQueueHandle workQueueHandle, ::TShouldStopPredicate shouldStop) {
        REQUIRE(workQueueHandle != nullptr);

        unsigned num_iter = 0;
        thread_exec_pushes.push_back("started");
        while (num_iter++ < MaxIterations && !shouldStop())
        {
            thread_exec_pushes.push_back("iteration");
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }
        thread_exec_pushes.push_back("success");
    });

    simple_thread.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    simple_thread.Stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    CHECK(thread_exec_pushes.size() >= 3);

    CHECK(thread_exec_pushes[0] == "started");
    for (size_t i = 0; i<thread_exec_pushes.size(); ++i)
    {
        CHECK(thread_exec_pushes[i] == "iteration");
    }
    CHECK(thread_exec_pushes[thread_exec_pushes.size() - 1] == "success");
}
