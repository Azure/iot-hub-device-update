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
#include <stdlib.h>     // rand
#include <sys/param.h>  // MIN/MAX
#include <unistd.h>

static time_t GetTimeSinceEpochInSeconds()
{
    struct timespec timeSinceEpoch;
    clock_gettime(CLOCK_REALTIME, &timeSinceEpoch);
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
time_t ADUC_Retry_Delay_Calculator(int additionalDelaySecs, unsigned int retries, long initialDelayUnitMilliSecs, long maxDelaySecs, double maxJitterPercent)
{
    double jitterPercent = (maxJitterPercent / 100.0) * (rand() / ((double)RAND_MAX));
    double delay = (pow(2, MIN(retries, ADUC_RETRY_MAX_RETRY_EXPONENT)) * (double)initialDelayUnitMilliSecs) / 1000.0;
    if (delay > maxDelaySecs)
    {
        delay = maxDelaySecs;
    }
    time_t retryTimestampSec = GetTimeSinceEpochInSeconds() + additionalDelaySecs + (unsigned long)(delay * (1 + jitterPercent));
    return retryTimestampSec;
}
