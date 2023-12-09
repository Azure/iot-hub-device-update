#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>
#include <aduc_worker_thread.hpp>

TEST_CASE("Worker Thread Tests", "[threads]")
{
    const unsigned MaxIterations = 10000;

    std::string data{ "" };

    ADUC::WorkerThread simple_thread( [&data](ADUC::TShouldStopPredicate shouldStop) {
        unsigned num_iter = 0;
        data = "started";
        while (num_iter++ < MaxIterations && !shouldStop())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }
        data = "success";
    });

    simple_thread.Start();
    simple_thread.Stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    CHECK(data == "success");
}
