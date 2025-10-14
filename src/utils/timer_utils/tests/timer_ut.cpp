/**
 * @file timer_ut.cpp
 * @brief The unit tests for timer utilities.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/defer.hpp"
#include "aduc/logging.h"
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
    ADUC_Logging_Init(ADUC_LOG_DEBUG, "timer_ut");
    aduc::Defer deferUninitLogging([]() { ADUC_Logging_Uninit(); });

    s_reset_test_metrics();

    AducTimerSignals signals = (AducTimerSignals){
        .onStart = s_on_start,
        .onStop = s_on_stop,
        .onTimeout = s_on_timeout,
    };
    AducTimer t = { 0 };
    REQUIRE(AducTimer_init(&t, signals, 50) == 0);
    REQUIRE(t.initialized);
    aduc::Defer deferUninitTimer([&t]() {
        AducTimer_Stop(&t);
        AducTimer_uninit(&t);
        CHECK_FALSE(t.initialized);
    });

    AducTimer_Start(&t, 200);
    usleep(250 * 1000);
    CHECK(_g_start_called);
    CHECK(_g_timeout_called);

    AducTimer_Stop(&t);
    usleep(100 * 1000);
    CHECK(_g_stop_called);
}

TEST_CASE("AducTimer reuse", "[timer]")
{
    ADUC_Logging_Init(ADUC_LOG_DEBUG, "timer_ut");
    aduc::Defer deferUninitLogging([]() { ADUC_Logging_Uninit(); });

    s_reset_test_metrics();
    AducTimerSignals signals = (AducTimerSignals){
        .onStart = s_on_start,
        .onStop = s_on_stop,
        .onTimeout = s_on_timeout,
    };
    AducTimer t = { 0 };
    REQUIRE(AducTimer_init(&t, signals, 50) == 0);
    REQUIRE(t.initialized);
    aduc::Defer deferUninitTimer([&t]() {
        AducTimer_Stop(&t);
        AducTimer_uninit(&t);
        CHECK_FALSE(t.initialized);
    });

    AducTimer_Start(&t, 100);
    CHECK(_g_start_called);

    usleep(125 * 1000);
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
    CHECK(_g_timeout_called);
}
