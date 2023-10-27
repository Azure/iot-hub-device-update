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
#include <aducpal/sys_time.h> // time_t
#include <pthread.h>
#include <stdbool.h>

EXTERN_C_BEGIN

#define TIME_SPAN_ONE_MINUTE_IN_SECONDS (60)
#define TIME_SPAN_FIVE_MINUTES_IN_SECONDS (5 * 60)
#define TIME_SPAN_FIFTEEN_SECONDS_IN_SECONDS (15)
#define TIME_SPAN_ONE_HOUR_IN_SECONDS (60 * 60)
#define TIME_SPAN_ONE_DAY_IN_SECONDS (24 * 60 * 60)

#define ADUC_RETRY_DEFAULT_INITIAL_DELAY_MS 1000 // 1 second
#define ADUC_RETRY_DEFAULT_MAX_BACKOFF_TIME_MS (60 * 1000) // 60 seconds
#define ADUC_RETRY_DEFAULT_MAX_JITTER_PERCENT 5
#define ADUC_RETRY_MAX_RETRY_EXPONENT 9
#define ADUC_RETRY_FALLBACK_WAIT_TIME_SEC 30
/**
 * @brief The data structure that contains information about the retry strategy.
 */
typedef struct tagADUC_Retry_Params
{
    unsigned int maxRetries; /**< Maximum number of retries */
    unsigned int maxDelaySecs; /**< Maximum wait time before retry (in seconds) */
    unsigned int fallbackWaitTimeSec; /**< The fallback time when regular timestamp calculation failed. */
    unsigned int initialDelayUnitMilliSecs; /**< Backoff factor (in milliseconds ) */
    double maxJitterPercent; /**< The maximum number of jitter percent (0 - 100)*/
} ADUC_Retry_Params;

/**
 * A function used for calculating a delay time before the next retry.
 */
typedef time_t (*ADUC_NEXT_RETRY_TIMESTAMP_CALC_FUNC)(
    int additionalDelaySecs,
    unsigned int retries,
    unsigned long initialDelayUnitMilliSecs,
    unsigned long maxDelaySecs,
    double maxJitterPercent);

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
time_t ADUC_Retry_Delay_Calculator(
    int additionalDelaySecs,
    unsigned int retries,
    unsigned long initialDelayUnitMilliSecs,
    unsigned long maxDelaySecs,
    double maxJitterPercent);

typedef enum tagADUC_Failure_Class
{
    ADUC_Failure_Class_None,
    ADUC_Failure_Class_Client_Transient,
    ADUC_Failure_Class_Client_Unrecoverable,
    ADUC_Failure_Class_Server_Transient,
    ADUC_Failure_Class_Server_Unrecoverable,
} ADUC_Failure_Class;

/**
 * @brief Get current time since epoch in seconds.
 */
time_t ADUC_GetTimeSinceEpochInSeconds();

typedef enum tagADUC_Retriable_Operation_State
{
    ADUC_Retriable_Operation_State_Destroyed = -4,
    ADUC_Retriable_Operation_State_Cancelled = -3,
    ADUC_Retriable_Operation_State_Failure = -2,
    ADUC_Retriable_Operation_State_Failure_Retriable = -1,
    ADUC_Retriable_Operation_State_NotStarted = 0,
    ADUC_Retriable_Operation_State_InProgress = 1,
    ADUC_Retriable_Operation_State_TimedOut = 2,
    ADUC_Retriable_Operation_State_Cancelling = 3,
    ADUC_Retriable_Operation_State_Expired = 4,
    ADUC_Retriable_Operation_State_Completed = 5,
} ADUC_Retriable_Operation_State;

typedef struct tagADUC_Retriable_Operation_Context ADUC_Retriable_Operation_Context;
struct tagADUC_Retriable_Operation_Context
{
    // Operation data
    void* operationName;
    void* data;

    // Custom functions
    void (*dataDestroyFunc)(ADUC_Retriable_Operation_Context* data);
    void (*operationDestroyFunc)(ADUC_Retriable_Operation_Context* context);
    bool (*doWorkFunc)(ADUC_Retriable_Operation_Context* context);
    bool (*cancelFunc)(ADUC_Retriable_Operation_Context* context);
    bool (*retryFunc)(ADUC_Retriable_Operation_Context* context, const ADUC_Retry_Params* retryParams);
    bool (*completeFunc)(ADUC_Retriable_Operation_Context* context);

    // Callbacks
    void (*onExpired)(ADUC_Retriable_Operation_Context* context);
    void (*onSuccess)(ADUC_Retriable_Operation_Context* context);
    void (*onFailure)(ADUC_Retriable_Operation_Context* context);
    void (*onRetry)(ADUC_Retriable_Operation_Context* context);

    // Configuration
    ADUC_Retry_Params* retryParams; // Array of retry parameters per class of errors.
    int retryParamsCount; // Number of elements in the 'retryParams' array.

    // Runtime data
    ADUC_Retriable_Operation_State state;
    ADUC_Failure_Class lastFailureClass;
    time_t nextExecutionTime; // Time when the operation should be executed
    time_t operationTimeoutSecs; // Timeout for the operation (in seconds) ( <1 means no timeout)

    time_t expirationTime; // Time when the 'operation' expires,
        // regardless of attempts or class of errors encountered.
        // (<0 means no expiration)

    unsigned int attemptCount; // Number of attempts
    unsigned int operationIntervalSecs; // Interval between operations (in seconds) ( <1 means no interval)

    time_t lastExecutionTime; // Time when the operation was last executed
    time_t lastFailureTime; // Time when the operation last failed
    time_t lastSuccessTime; // Time when the operation last succeeded

    void* lastErrorContext; // Last error context

    const void* commChannelHandle; // Handle to the communication channel used for the operation
};

void ADUC_Retriable_Operation_Init(ADUC_Retriable_Operation_Context* context, bool startNow);
bool ADUC_Retriable_Operation_DoWork(ADUC_Retriable_Operation_Context* context);
bool ADUC_Retriable_Operation_Cancel(ADUC_Retriable_Operation_Context* context);
bool ADUC_Retriable_Set_State(ADUC_Retriable_Operation_Context* context, ADUC_Retriable_Operation_State state);

EXTERN_C_END

#endif // RETRY_UTILS_H
