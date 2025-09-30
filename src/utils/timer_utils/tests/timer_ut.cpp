#include "aduc/timer.h"
#include <catch2/catch_all.hpp>
#include <stdbool.h>
#include <unistd.h>

bool _g_start_called = false;
bool _g_stop_called = false;
bool _g_timeout_called = false;

static void s_reset_test_metrics()
{
    _g_start_called = false;
    _g_stop_called = false;
    _g_timeout_called = false;
}

static void s_on_start()
{
    _g_start_called = true;
}

static void s_on_stop()
{
    _g_stop_called = true;
}

static void s_on_timeout()
{
    _g_timeout_called = true;
}

TEST_CASE("AducTimer callback on timeout", "[timer]")
{
    s_reset_test_metrics();
    AducTimer t = (AducTimer){ .startTime = { 0, 0 },
                               .waitTimeMs = 0,
                               .signals = {
                                   .onStart = s_on_start,
                                   .onStop = s_on_stop,
                                   .onTimeout = s_on_timeout,
                               } };

    AducTimer_Start(&t, 200);
    CHECK(_g_start_called);

    usleep(250 * 1000);

    AducTimer_Update(&t);

    CHECK(_g_timeout_called);

    AducTimer_Stop(&t);
    CHECK(_g_stop_called);
}

TEST_CASE("AducTimer reuse", "[timer]")
{
    s_reset_test_metrics();
    AducTimer t = (AducTimer){ .startTime = { 0, 0 },
                               .waitTimeMs = 0,
                               .signals = {
                                   .onStart = s_on_start,
                                   .onStop = s_on_stop,
                                   .onTimeout = s_on_timeout,
                               } };

    AducTimer_Start(&t, 100);
    CHECK(_g_start_called);

    usleep(125 * 1000);

    AducTimer_Update(&t);
    CHECK(_g_timeout_called);

    AducTimer_Stop(&t);
    CHECK(_g_stop_called);

    s_reset_test_metrics();
    AducTimer_Start(&t, 10);
    CHECK(_g_start_called);
    AducTimer_Stop(&t);

    s_reset_test_metrics();
    AducTimer_Start(&t, 25);
    CHECK(_g_start_called);
    usleep(30 * 1000);
    AducTimer_Update(&t);
    CHECK(_g_timeout_called);
}

TEST_CASE("AducTimer mult ticks", "[timer]")
{
    s_reset_test_metrics();
    AducTimer t = (AducTimer){ .startTime = { 0, 0 },
                               .waitTimeMs = 0,
                               .signals = {
                                   .onStart = s_on_start,
                                   .onStop = s_on_stop,
                                   .onTimeout = s_on_timeout,
                               } };

    AducTimer_Start(&t, 500);
    CHECK(_g_start_called);

    AducTimer_Update(&t);
    CHECK_FALSE(_g_timeout_called);
    AducTimer_Update(&t);
    CHECK_FALSE(_g_timeout_called);
    AducTimer_Update(&t);
    CHECK_FALSE(_g_timeout_called);

    usleep(500 * 1000);
    AducTimer_Update(&t);
    CHECK_FALSE(_g_stop_called);
    CHECK(_g_timeout_called);
}
