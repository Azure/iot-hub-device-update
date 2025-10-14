/**
 * @file timer.c
 * @brief The impl for timer utilities.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/timer.h"
#include "aduc/logging.h"

#include <string.h>
#include <time.h>

#include <aduc/aduc_banned.h>

static inline bool _is_running(const AducTimer* t)
{
#ifdef DBG_TIMER_UTILS
    Log_Debug("StartTime: %ld, %ld, WaitTimeMs: %ld", t->startTime.tv_sec, t->startTime.tv_nsec, t->waitTimeMs);
#endif
    return ((t->startTime.tv_sec != 0 || t->startTime.tv_nsec != 0) && t->waitTimeMs != 0);
}

static inline bool _wait_exceeded(const AducTimer* t)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    long long start_ms = t->startTime.tv_sec * 1000LL + t->startTime.tv_nsec / 1000000LL;
    long long now_ms = now.tv_sec * 1000LL + now.tv_nsec / 1000000LL;

#ifdef DBG_TIMER_UTILS
    Log_Debug("Now: %lld, Start: %lld, Wait: %ld", now_ms, start_ms, t->waitTimeMs);
#endif
    return (now_ms - start_ms) >= t->waitTimeMs;
}

static inline void _reset(AducTimer* t)
{
#ifdef DBG_TIMER_UTILS
    Log_Debug("Resetting timer");
#endif
    t->startTime = (struct timespec){ 0, 0 };
    t->waitTimeMs = 0;
}

static void* s_timerProc(void* context)
{
    AducTimer* t = (AducTimer*)context;
    if (t == NULL)
    {
        Log_Error("Timer context is NULL");
        return NULL;
    }

    while (t->timerThreadRunning)
    {
        AducTimer_Update(t);
        usleep(t->updateIntervalMs * 1000); // sleep for update interval
    }

    return NULL;
}

int AducTimer_init(AducTimer* t, AducTimerSignals s, unsigned update_interval_ms)
{
    if (t == NULL)
    {
        return -1;
    }

    if (pthread_mutex_init(&t->mut, NULL) != 0)
    {
        Log_Error("Failed to initialize timer mutex");
        return -1;
    }

    t->startTime = (struct timespec){ 0, 0 };
    t->waitTimeMs = 0;
    t->signals = s;
    t->updateIntervalMs = update_interval_ms;
    t->initialized = true;

#ifdef DBG_TIMER_UTILS
    Log_Debug("Initialized timer");
#endif
    return 0;
}

void AducTimer_uninit(AducTimer* t)
{
    if (t == NULL)
    {
        return;
    }
    AducTimer_Stop(t);
    pthread_mutex_destroy(&t->mut);
    memset(t, 0, sizeof(AducTimer));
#ifdef DBG_TIMER_UTILS
    Log_Debug("Uninitialized timer");
#endif
}

bool AducTimer_IsTimedOut(AducTimer* t)
{
#ifdef DBG_TIMER_UTILS
    Log_Debug("IsTimedOut called: result=%d", !_is_running(t) || _wait_exceeded(t));
#endif
    pthread_mutex_lock(&t->mut);
    bool timed_out = !_is_running(t) || _wait_exceeded(t);
    pthread_mutex_unlock(&t->mut);
    return timed_out;
}

int AducTimer_Start(AducTimer* t, unsigned w)
{
#ifdef DBG_TIMER_UTILS
    Log_Debug("Starting timer for %u ms", w);
#endif
    pthread_mutex_lock(&t->mut);
    _reset(t);
    t->waitTimeMs = w;
    clock_gettime(CLOCK_MONOTONIC, &t->startTime);
    if (w != 0 && t->updateIntervalMs != 0) // use zero if want to manually update pump the timer
    {
        t->timerThreadRunning = true;
        int res = pthread_create(&t->timerThread, NULL, (void* (*)(void*))s_timerProc, t);
        if (res != 0)
        {
            Log_Error("Failed to create timer thread");
            t->timerThreadRunning = false;
            return -1;
        }
    }
    pthread_mutex_unlock(&t->mut);

    if (t->signals.onStart)
    {
        t->signals.onStart();
    }
    return 0;
}

void AducTimer_Update(AducTimer* t)
{
#ifdef DBG_TIMER_UTILS
    Log_Debug("Updating timer");
#endif
    if (_is_running(t))
    {
        if (_wait_exceeded(t))
        {
            _reset(t); // auto-stop the timer until next start.
            if (t->signals.onTimeout)
            {
                Log_Info("Timer timed out, invoking 'onTimeout' callback");
                t->signals.onTimeout();
            }
        }
    }
}

void AducTimer_Stop(AducTimer* t)
{
#ifdef DBG_TIMER_UTILS
    Log_Debug("Stopping timer");
#endif
    if (t == NULL)
    {
        return;
    }
    pthread_mutex_lock(&t->mut);
    t->timerThreadRunning = false;
    _reset(t);
    AducTimerCallback onStop = t->signals.onStop;
    pthread_mutex_unlock(&t->mut);
    if (onStop)
    {
#ifdef DBG_TIMER_UTILS
        Log_Debug("Invoking onStop callback");
#endif
        onStop();
    }
}
