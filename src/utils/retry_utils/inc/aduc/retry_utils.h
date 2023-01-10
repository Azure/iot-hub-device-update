/**
 * @file retry_utils.h
 * @brief Utilities for retry timestamp calculation.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef RETRY_UTILS_H
#define RETRY_UTILS_H

#include "aduc/c_utils.h"
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>

EXTERN_C_BEGIN

#define TIME_SPAN_ONE_MINUTE_IN_SECONDS (60)
#define TIME_SPAN_FIVE_MINUTES_IN_SECONDS (5 * 60)
#define TIME_SPAN_FIFTEEN_SECONDS_IN_SECONDS (15)
#define TIME_SPAN_ONE_HOUR_IN_SECONDS (60 * 60)
#define TIME_SPAN_ONE_DAY_IN_SECONDS (24 * 60 * 60)

#define MILLISECONDS_TO_NANOSECONDS(ms) ((ms) * 1000000)

#define ADUC_RETRY_DEFAULT_INITIAL_DELAY_MS 1000                    // 1 second
#define ADUC_RETRY_DEFAULT_MAX_BACKOFF_TIME_MS (60 * 1000) // 60 seconds
#define ADUC_RETRY_DEFAULT_MAX_JITTER_PERCENT 5
#define ADUC_RETRY_MAX_RETRY_EXPONENT 9
#define ADUC_RETRY_FALLBACK_WAIT_TIME_SEC 30
/**
 * @brief The data structure that contains information about the retry strategy.
 */
typedef struct _tagADUC_Retry_Params
{
    unsigned int maxRetries;            /**< Maximum number of retries */
    unsigned long maxDelaySecs;         /**< Maximum wait time before retry (in seconds) */
    unsigned long fallbackWaitTimeSec;  /**< The fallback time when regular timestamp calculation failed. */
    unsigned long initialDelayUnitMilliSecs;       /**< Backoff factor (in milliseconds ) */
    double maxJitterPercent;            /**< The maximum number of jitter percent (0 - 100)*/
} ADUC_Retry_Params;

/**
 * A function used for calculating a delay time before the next retry.
 */
typedef time_t (*ADUC_NEXT_RETRY_TIMESTAMP_CALC_FUNC)(
    int additionalDelaySecs, unsigned int retries, long initialDelayUnitMilliSecs, long maxDelaySecs, double maxJitterPercent);

/**
 * @brief A default retry delay calculator function.
 *
 * @param additionalDelaySecs Additional delay time, to be added on top of calculated time, in seconds.
 * @param retries A current retries count.
 * @param initialDelayUnitMilliSecs An initial delay time that is used in the calculation function, in milliseconds.
 * @param maxDelaySecs  A max delay time, in seconds.
 * @param maxJitterPercent A maximum jitter percentage.
 * @return time_t Return a timestamp (since epoch) for the next retry.
 */
time_t ADUC_Retry_Delay_Calculator(int additionalDelaySecs, unsigned int retries, long initialDelayUnitMilliSecs, long maxDelaySecs, double maxJitterPercent);

EXTERN_C_END

#endif // RETRY_UTILS_H
