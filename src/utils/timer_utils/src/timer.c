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

    while (true)
    {
        // Read the running flag and update interval under the mutex so we
        // don't race with Start/Stop mutating them.
        pthread_mutex_lock(&t->mut);
        bool running = t->timerThreadRunning;
        unsigned int interval_ms = t->updateIntervalMs;
        pthread_mutex_unlock(&t->mut);

        if (!running)
        {
            break;
        }

        AducTimer_Update(t);
        usleep(interval_ms * 1000); // sleep for update interval
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
    t->timerThreadRunning = false;
    t->threadCreated = false;
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
    pthread_mutex_lock(&t->mut);
    bool timed_out = !_is_running(t) || _wait_exceeded(t);
#ifdef DBG_TIMER_UTILS
    Log_Debug("IsTimedOut called: result=%d", timed_out);
#endif
    pthread_mutex_unlock(&t->mut);
    return timed_out;
}

/**
 * Internal: signal the polling thread to exit and join it. Does NOT
 * invoke the onStop callback (that is done by AducTimer_Stop only).
 * Used by both Stop (public) and Start (to ensure idempotency).
 */
static void _stop_thread(AducTimer* t)
{
    pthread_mutex_lock(&t->mut);
    bool created = t->threadCreated;
    pthread_t thr = t->timerThread;
    t->timerThreadRunning = false;
    t->threadCreated = false;
    _reset(t);
    pthread_mutex_unlock(&t->mut);

    if (created)
    {
        // Join outside the mutex so the thread can finish its current
        // iteration (which may itself take the mutex via AducTimer_Update).
        pthread_join(thr, NULL);
    }
}

int AducTimer_Start(AducTimer* t, unsigned w)
{
    if (t == NULL)
    {
        return -1;
    }

#ifdef DBG_TIMER_UTILS
    Log_Debug("Starting timer for %u ms", w);
#endif

    // Idempotent against a prior Start that wasn't matched by a Stop:
    // tear down the previous polling thread before creating a new one.
    // This avoids leaking the previous pthread_t handle and the data
    // races / use-after-free that result from two threads polling the
    // same AducTimer concurrently.
    _stop_thread(t);

    pthread_mutex_lock(&t->mut);
    _reset(t);
    t->waitTimeMs = w;
    clock_gettime(CLOCK_MONOTONIC, &t->startTime);
    if (w != 0 && t->updateIntervalMs != 0) // use zero if want to manually update pump the timer
    {
        t->timerThreadRunning = true;
        int res = pthread_create(&t->timerThread, NULL, s_timerProc, t);
        if (res != 0)
        {
            Log_Error("Failed to create timer thread");
            t->timerThreadRunning = false;
            _reset(t);
            pthread_mutex_unlock(&t->mut);
            return -1;
        }
        t->threadCreated = true;
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
    if (t == NULL)
    {
        return;
    }

    AducTimerCallback onTimeout = NULL;

    pthread_mutex_lock(&t->mut);
    if (_is_running(t) && _wait_exceeded(t))
    {
        _reset(t); // auto-stop the timer until next start.
        onTimeout = t->signals.onTimeout;
    }
    pthread_mutex_unlock(&t->mut);

    if (onTimeout != NULL)
    {
        Log_Info("Timer timed out, invoking 'onTimeout' callback");
        onTimeout();
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

    _stop_thread(t);

    // signals is set at init time and never mutated, so reading
    // onStop outside the lock is safe.
    AducTimerCallback onStop = t->signals.onStop;
    if (onStop)
    {
#ifdef DBG_TIMER_UTILS
        Log_Debug("Invoking onStop callback");
#endif
        onStop();
    }
}
