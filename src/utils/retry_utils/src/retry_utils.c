/**
 * @file retry_utils.c
 * @brief Utilities for retry timestamp calculation.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/retry_utils.h"

#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h> // rand

#include <aducpal/time.h> // clock_gettime

#ifndef MIN
#    define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

time_t ADUC_GetTimeSinceEpochInSeconds()
{
    struct timespec timeSinceEpoch;
    ADUCPAL_clock_gettime(CLOCK_REALTIME, &timeSinceEpoch);
    return timeSinceEpoch.tv_sec;
}

/**
 * @brief The default function for calculating the next retry timestamp based on current time (since epoch) and input parameters,
 *        using exponential backoff with jitter algorithm.
 *
 * Algorithm:
 *      next-retry-timestamp = nowTimeSec + additionalDelaySecs + (MIN( ( (2 ^ MIN(MAX_RETRY_EXPONENT, retries))) * initialDelayUnitMilliSecs) / 1000), maxDelaySecs ) * (1 + jitter))
 *
 *      where:
 *         -  jitter =  (maxJitterPercent / 100.0) * (rand() / RAND_MAX)
 *         -  MAX_RETRY_EXPONENT help avoid large exponential value (recommended value is 9)
 *         -  additionalDelaySecs can be customized to suits different type of http response error
 *
 * @param additionalDelaySecs An additional delay to add to the regular delay time.
 * @param retries The current retries count.
 * @param initialDelayUnitMilliSecs A time unit, in milliseconds, for backoff logic.
 * @param maxDelaySecs  The maximum delay time before retrying. This is useful for the scenario where the delay time cannot be too long.
 * @param maxJitterPercent The maximum jitter percentage. This must be between 0 and 100
 *
 * @return time_t Returns the next retry timestamp in seconds (since epoch).
 */
time_t ADUC_Retry_Delay_Calculator(
    int additionalDelaySecs,
    unsigned int retries,
    unsigned long initialDelayUnitMilliSecs,
    unsigned long maxDelaySecs,
    double maxJitterPercent)
{
    double jitterPercent = (maxJitterPercent / 100.0) * (rand() / ((double)RAND_MAX));
    double delay = (pow(2, MIN(retries, ADUC_RETRY_MAX_RETRY_EXPONENT)) * (double)initialDelayUnitMilliSecs) / 1000.0;
    if ((unsigned long)delay > maxDelaySecs)
    {
        delay = (double)maxDelaySecs;
    }
    time_t retryTimestampSec = (time_t)(
        ADUC_GetTimeSinceEpochInSeconds() + (time_t)additionalDelaySecs + (time_t)(delay * (1 + jitterPercent)));
    return retryTimestampSec;
}

void ADUC_Retriable_Operation_Init(ADUC_Retriable_Operation_Context* context, bool startNow)
{
    if (context == NULL)
    {
        return;
    }

    context->state = ADUC_Retriable_Operation_State_NotStarted;
    context->nextExecutionTime = 0;
    context->expirationTime = 0;
    context->attemptCount = 0;

    if (startNow)
    {
        context->nextExecutionTime = ADUC_GetTimeSinceEpochInSeconds();
    }
}

/**
 * @brief Perform a retriable operation.
 */
bool ADUC_Retriable_Operation_DoWork(ADUC_Retriable_Operation_Context* context)
{
    if (context == NULL)
    {
        return false;
    }

    time_t nowTime = ADUC_GetTimeSinceEpochInSeconds();

    switch (context->state)
    {
    case ADUC_Retriable_Operation_State_Completed:
    case ADUC_Retriable_Operation_State_Failure:
    case ADUC_Retriable_Operation_State_Cancelled:
    case ADUC_Retriable_Operation_State_Destroyed:
        // Nothing to do.
        return true;

    case ADUC_Retriable_Operation_State_NotStarted:
    case ADUC_Retriable_Operation_State_InProgress:
    case ADUC_Retriable_Operation_State_Expired:
    case ADUC_Retriable_Operation_State_Failure_Retriable:
    case ADUC_Retriable_Operation_State_Cancelling:
        // continue.
        break;
    case ADUC_Retriable_Operation_State_TimedOut:
        // continue.
        break;
    }

    bool jobResult = false;

    // The job took too long? Expire it.
    if (nowTime >= context->expirationTime)
    {
        context->state = ADUC_Retriable_Operation_State_Expired;
        if (context->onExpired)
        {
            // NOTE: expecting the worker to handle the expired state.
            // This including update the nextExecutionTime and related timestamps and state,
            // if try is still allowed.
            context->onExpired(context);
        }
    }

    // If it's time to do work, do it.
    if (nowTime >= context->nextExecutionTime)
    {
        // NOTE: perform the worker's job.
        // The worker is responsible for updating the nextExecutionTime and related timestamps and state.
        if (context->doWorkFunc != NULL)
        {
            jobResult = context->doWorkFunc(context);
        }
    }

    return jobResult;
}

bool ADUC_Retriable_Set_State(ADUC_Retriable_Operation_Context* context, ADUC_Retriable_Operation_State state)
{
    if (context == NULL)
    {
        return false;
    }

    context->state = state;
    return true;
}

bool ADUC_Retriable_Operation_Cancel(ADUC_Retriable_Operation_Context* context)
{
    if (context == NULL)
    {
        return false;
    }

    switch (context->state)
    {
    case ADUC_Retriable_Operation_State_NotStarted:
    case ADUC_Retriable_Operation_State_InProgress:
    case ADUC_Retriable_Operation_State_Expired:
    case ADUC_Retriable_Operation_State_Failure_Retriable:
        ADUC_Retriable_Set_State(context, ADUC_Retriable_Operation_State_Cancelling);
        return true;
    default:
        return false;
    }

    return false;
}
