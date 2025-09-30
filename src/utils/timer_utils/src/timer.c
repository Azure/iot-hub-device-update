
#include "aduc/timer.h"
#include <aduc/aduc_banned.h>
#include <time.h>

static inline bool _is_running(const AducTimer* t)
{
    return ((t->startTime.tv_sec != 0 || t->startTime.tv_nsec != 0) && t->waitTimeMs != 0);
}

static inline bool _wait_exceeded(const AducTimer* t)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    long long start_ms = t->startTime.tv_sec * 1000LL + t->startTime.tv_nsec / 1000000LL;
    long long now_ms = now.tv_sec * 1000LL + now.tv_nsec / 1000000LL;

    return (now_ms - start_ms) >= t->waitTimeMs;
}

static inline void _reset(AducTimer* t)
{
    t->startTime = (struct timespec){ 0, 0 };
    t->waitTimeMs = 0;
}

bool AducTimer_IsTimedOut(const AducTimer* t)
{
    return !_is_running(t) || _wait_exceeded(t);
}

void AducTimer_Start(AducTimer* t, unsigned w)
{
    t->waitTimeMs = w;
    clock_gettime(CLOCK_MONOTONIC, &t->startTime);
    if (t->signals.onStart)
    {
        t->signals.onStart();
    }
}

void AducTimer_Update(AducTimer* t)
{
    if (_is_running(t) && _wait_exceeded(t))
    {
        _reset(t);
        if (t->signals.onTimeout)
        {
            t->signals.onTimeout();
        }
    }
}

void AducTimer_Stop(AducTimer* t)
{
    _reset(t);
    if (t->signals.onStop)
    {
        t->signals.onStop();
    }
}
