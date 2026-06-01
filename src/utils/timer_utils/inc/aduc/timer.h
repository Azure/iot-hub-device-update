/**
 * @file timer.h
 * @brief The header for timer utilities.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_TIMER_H
#define ADUC_TIMER_H
#include "aduc/c_utils.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

EXTERN_C_BEGIN

typedef void (*AducTimerCallback)();

typedef struct tagAducTimerSignals
{
    AducTimerCallback onStart;
    AducTimerCallback onStop;
    AducTimerCallback onTimeout;
} AducTimerSignals;

typedef struct tagAducTimer
{
    struct timespec startTime;
    time_t waitTimeMs;
    AducTimerSignals signals;
    unsigned int updateIntervalMs;
    pthread_t timerThread;
    pthread_mutex_t mut;
    bool timerThreadRunning;
    bool threadCreated;
    bool initialized;
} AducTimer;

/**
 * Threading contract:
 *  - Start/Stop are thread-safe with respect to each other and to Update.
 *  - Stop blocks until the polling thread has exited (up to one
 *    updateIntervalMs sleep cycle).
 *  - Start is idempotent: if a previous Start has not been matched by a
 *    Stop, Start will internally stop and join the previous polling
 *    thread before starting a new one (the onStop callback is NOT
 *    invoked in this case, only on an explicit Stop).
 *  - Signal callbacks (onStart / onStop / onTimeout) MUST NOT call
 *    Start, Stop, or uninit on the same timer. The timer thread joins
 *    itself in that case, which is undefined behavior.
 */

int AducTimer_init(AducTimer* t, AducTimerSignals s, unsigned update_interval_ms);
void AducTimer_uninit(AducTimer* t);
bool AducTimer_IsTimedOut(AducTimer* t);
int AducTimer_Start(AducTimer* t, unsigned w);
void AducTimer_Update(AducTimer* t);
void AducTimer_Stop(AducTimer* t);

EXTERN_C_END

#endif // ADUC_TIMER_H
