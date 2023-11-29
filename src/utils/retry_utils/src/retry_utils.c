/**
 * @file retry_utils.c
 * @brief Utilities for retry timestamp calculation.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/retry_utils.h"

#include "parson_json_utils.h" // ADUC_JSON_GetUnsignedIntegerField
#include <aduc/logging.h>
#include <aducpal/time.h> // clock_gettime, CLOCK_REALTIME
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h> // INT32_MAX
#include <stdlib.h> // rand

time_t ADUC_GetTimeSinceEpochInSeconds()
{
    struct timespec timeSinceEpoch;
    ADUCPAL_clock_gettime(CLOCK_REALTIME, &timeSinceEpoch);
    return timeSinceEpoch.tv_sec;
}

typedef struct tagADUC_RETRY_PARAMS_MAP
{
    const char* name;
    int index;
} ADUC_RETRY_PARAMS_MAP;

static const ADUC_RETRY_PARAMS_MAP s_retryParamsMap[] = {
    { "default", ADUC_RETRY_PARAMS_INDEX_DEFAULT },
    { "clientTransient", ADUC_RETRY_PARAMS_INDEX_CLIENT_TRANSIENT },
    { "clientUnrecoverable", ADUC_RETRY_PARAMS_INDEX_CLIENT_UNRECOVERABLE },
    { "serviceTransient", ADUC_RETRY_PARAMS_INDEX_SERVICE_TRANSIENT },
    { "serviceUnrecoverable", ADUC_RETRY_PARAMS_INDEX_SERVICE_UNRECOVERABLE }
};

static const int s_RetryParamsMapSize = sizeof(s_retryParamsMap) / sizeof(s_retryParamsMap[0]);

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
    const double jitterPercent = (maxJitterPercent / 100.0) * (rand() / ((double)RAND_MAX));
    const unsigned min = retries < ADUC_RETRY_MAX_RETRY_EXPONENT ? retries : ADUC_RETRY_MAX_RETRY_EXPONENT;

    double delay = (pow(2, min) * (double)initialDelayUnitMilliSecs) / 1000.0;
    time_t retryTimestampSec = 0;

    if ((unsigned long)delay > maxDelaySecs)
    {
        delay = (double)maxDelaySecs;
    }

    retryTimestampSec = (time_t)(
        ADUC_GetTimeSinceEpochInSeconds() + (time_t)additionalDelaySecs + (time_t)(delay * (1 + jitterPercent)));

    return retryTimestampSec;
}

/**
 * @brief Initialize the Retriale operation context.
 *
 * @param context The context to be initialized.
 * @param startNow Whether to set nextExecutionTime to now.
 */
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
 *
 * @param context The retriable operation context.
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

int RetryUtils_GetRetryParamsMapSize()
{
    return s_RetryParamsMapSize;
}

void ReadRetryParamsArrayFromAgentConfigJson(
    ADUC_Retriable_Operation_Context* context, JSON_Value* agentJsonValue, int retryParamsMapSize)
{
    const char* infoFormatString = "Failed to read '%s.%s' from agent config. Using default value (%d)";

    for (int i = 0; i < retryParamsMapSize; i++)
    {
        ADUC_Retry_Params* params = &context->retryParams[i];
        JSON_Value* retryParams = json_object_dotget_value(json_object(agentJsonValue), s_retryParamsMap[i].name);
        if (retryParams == NULL)
        {
            Log_Info("Retry params for '%s' is not specified. Using default values.", s_retryParamsMap[i].name);
            params->maxRetries = DEFAULT_ENR_REQ_OP_MAX_RETRIES;
            params->maxDelaySecs = context->operationIntervalSecs;
            params->fallbackWaitTimeSec = context->operationIntervalSecs;
            params->initialDelayUnitMilliSecs = DEFAULT_ENR_REQ_OP_INITIAL_DELAY_MILLISECONDS;
            params->maxJitterPercent = DEFAULT_ENR_REQ_OP_MAX_JITTER_PERCENT;
            continue;
        }

        if (!ADUC_JSON_GetUnsignedIntegerField(retryParams, SETTING_KEY_ENR_REQ_OP_MAX_RETRIES, &params->maxRetries))
        {
            Log_Info(
                infoFormatString,
                s_retryParamsMap[i].name,
                SETTING_KEY_ENR_REQ_OP_MAX_RETRIES,
                DEFAULT_ENR_REQ_OP_MAX_RETRIES);
            params->maxRetries = DEFAULT_ENR_REQ_OP_MAX_RETRIES;
        }

        if (!ADUC_JSON_GetUnsignedIntegerField(
                retryParams, SETTING_KEY_ENR_REQ_OP_MAX_WAIT_SECONDS, &params->maxDelaySecs))
        {
            Log_Info(
                infoFormatString,
                s_retryParamsMap[i].name,
                SETTING_KEY_ENR_REQ_OP_MAX_WAIT_SECONDS,
                context->operationIntervalSecs);
            params->maxDelaySecs = context->operationIntervalSecs;
        }

        if (!ADUC_JSON_GetUnsignedIntegerField(
                retryParams, SETTING_KEY_ENR_REQ_OP_FALLBACK_WAITTIME_SECONDS, &params->fallbackWaitTimeSec))
        {
            Log_Info(
                infoFormatString,
                s_retryParamsMap[i].name,
                SETTING_KEY_ENR_REQ_OP_FALLBACK_WAITTIME_SECONDS,
                context->operationIntervalSecs);
            params->fallbackWaitTimeSec = context->operationIntervalSecs;
        }

        if (!ADUC_JSON_GetUnsignedIntegerField(
                retryParams, SETTING_KEY_ENR_REQ_OP_INITIAL_DELAY_MILLISECONDS, &params->initialDelayUnitMilliSecs))
        {
            Log_Info(
                infoFormatString,
                s_retryParamsMap[i].name,
                SETTING_KEY_ENR_REQ_OP_INITIAL_DELAY_MILLISECONDS,
                DEFAULT_ENR_REQ_OP_INITIAL_DELAY_MILLISECONDS);
            params->initialDelayUnitMilliSecs = DEFAULT_ENR_REQ_OP_INITIAL_DELAY_MILLISECONDS;
        }

        unsigned int value = 0;
        if (!ADUC_JSON_GetUnsignedIntegerField(retryParams, SETTING_KEY_ENR_REQ_OP_MAX_JITTER_PERCENT, &value))
        {
            Log_Info(
                infoFormatString,
                s_retryParamsMap[i].name,
                SETTING_KEY_ENR_REQ_OP_MAX_JITTER_PERCENT,
                DEFAULT_ENR_REQ_OP_MAX_JITTER_PERCENT);
            value = DEFAULT_ENR_REQ_OP_MAX_JITTER_PERCENT;
        }
        params->maxJitterPercent = (double)value;
    }
}
