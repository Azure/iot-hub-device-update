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
} AducTimer;

bool AducTimer_IsTimedOut(const AducTimer* t);
void AducTimer_Start(AducTimer* t, unsigned w);
void AducTimer_Update(AducTimer* t);
void AducTimer_Stop(AducTimer* t);

EXTERN_C_END

#endif // ADUC_TIMER_H
