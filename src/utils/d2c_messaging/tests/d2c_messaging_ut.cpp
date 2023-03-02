
#include "aduc/client_handle.h"
#include "aduc/d2c_messaging.h"
#include "aduc/retry_utils.h"

#include <catch2/catch.hpp>
#include <stdexcept> // runtime_error
#include <string.h>
#include <sys/param.h> // *_MIN/*_MAX
#include <sys/time.h> // nanosleep
#include <unistd.h> // usleep

static ADUC_D2C_HttpStatus_Retry_Info g_httpStatusRetryInfo_fast_speed[]{
    /* Success responses, no retries needed */
    { .httpStatusMin = 200,
      .httpStatusMax = 299,
      .additionalDelaySecs = 0,
      .retryTimestampCalcFunc = nullptr,
      .maxRetry = 0 },

    { .httpStatusMin = 400,
      .httpStatusMax = 499,
      .additionalDelaySecs = 0,
      .retryTimestampCalcFunc = ADUC_Retry_Delay_Calculator,
      .maxRetry = INT_MAX },

    /* Catch all */
    { .httpStatusMin = 0,
      .httpStatusMax = INT_MAX,
      .additionalDelaySecs = 0,
      .retryTimestampCalcFunc = ADUC_Retry_Delay_Calculator,
      .maxRetry = INT_MAX }
};

/**
 * @brief The default retry strategy for all Device-to-Cloud message requests to the Azure IoT Hub.
 */
static ADUC_D2C_RetryStrategy g_defaultRetryStrategy_fast_speed = {
    .httpStatusRetryInfo = g_httpStatusRetryInfo_fast_speed,
    .httpStatusRetryInfoSize = 3,
    .maxRetries = INT_MAX,
    .maxDelaySecs = 1, // 1 seconds
    .fallbackWaitTimeSec = 1, // 20 ms.
    .initialDelayUnitMilliSecs = 10 // 50 ms.
};

// Bad retry strategy - retry, but no calc function pointer.
static ADUC_D2C_HttpStatus_Retry_Info g_httpStatusRetryInfo_no_calc_func[]{
    /* Success responses, no retries needed */
    { .httpStatusMin = 200,
      .httpStatusMax = 299,
      .additionalDelaySecs = 0,
      .retryTimestampCalcFunc = nullptr,
      .maxRetry = 0 },

    /* Server error, no retries needed, but no retryTimestampCalcFunc specified.*/
    { .httpStatusMin = 500,
      .httpStatusMax = 599,
      .additionalDelaySecs = 0,
      .retryTimestampCalcFunc = nullptr,
      .maxRetry = 0 },

    /* Mock errors, retry required, but no retryTimestampCalcFunc specified.*/
    { .httpStatusMin = 600,
      .httpStatusMax = 699,
      .additionalDelaySecs = 0,
      .retryTimestampCalcFunc = nullptr,
      .maxRetry = INT_MAX },

    /* Catch all */
    { .httpStatusMin = 0,
      .httpStatusMax = INT_MAX,
      .additionalDelaySecs = 0,
      .retryTimestampCalcFunc = ADUC_Retry_Delay_Calculator,
      .maxRetry = INT_MAX }
};

/**
 * @brief The default retry strategy for all Device-to-Cloud message requests to the Azure IoT Hub.
 */
static ADUC_D2C_RetryStrategy g_defaultRetryStrategy_no_calc_func = { .httpStatusRetryInfo =
                                                                          g_httpStatusRetryInfo_no_calc_func,
                                                                      .httpStatusRetryInfoSize = 3,
                                                                      .maxRetries = INT_MAX,
                                                                      .maxDelaySecs = 1 * 24 * 60 * 60, // 1 day
                                                                      .fallbackWaitTimeSec = 1,
                                                                      .initialDelayUnitMilliSecs = 1000 };

typedef struct _tagMockCloudBehavior
{
    time_t delayBeforeResponseMS; // in ms.
    int httpStatus;
} MockCloudBehavior;

class PThreadMutex
{
public:
    PThreadMutex(const pthread_mutexattr_t* attr = nullptr)
    {
        if (pthread_mutex_init(&_mutex, attr) != 0)
        {
            throw std::runtime_error("pthread_mutex_init failed");
        }
    }
    ~PThreadMutex()
    {
        pthread_mutex_destroy(&_mutex);
    }

    pthread_mutex_t* ptr()
    {
        return &_mutex;
    }

    void lock()
    {
        pthread_mutex_lock(&_mutex);
    }

    void unlock()
    {
        pthread_mutex_unlock(&_mutex);
    }

private:
    pthread_mutex_t _mutex;
};

class PThreadCond
{
public:
    PThreadCond(const pthread_condattr_t* attr = nullptr)
    {
        if (pthread_cond_init(&_cond, attr) != 0)
        {
            throw std::runtime_error("pthread_cond_init failed");
        }
    }
    ~PThreadCond()
    {
        pthread_cond_destroy(&_cond);
    }

    void signal()
    {
        pthread_cond_signal(&_cond);
    }

    void wait(PThreadMutex& mutex)
    {
        pthread_cond_wait(&_cond, mutex.ptr());
    }

private:
    pthread_cond_t _cond;
};

static PThreadMutex g_cloudBehaviorMutex;
static PThreadMutex g_doWorkMutex;
const MockCloudBehavior* g_cloudBehavior = nullptr;
size_t g_cloudBehaviorCount = 0;
size_t g_cloudBehaviorIndex = 0;

ADUC_C2D_RESPONSE_HANDLER_FUNCTION g_c2dResponseHandlerFunc = nullptr;
int g_attempts = 0;

static PThreadCond g_d2cMessageProcessedCond;
static PThreadMutex g_cloudServiceMutex;
static PThreadMutex g_testCaseSyncMutex;

void* mock_msg_process_thread_routine(void* context)
{
    g_cloudBehaviorMutex.lock();
    if (g_cloudBehaviorIndex >= g_cloudBehaviorCount)
    {
        INFO("Invalid g_cloudBehaviorIndex!");
        g_cloudBehaviorMutex.unlock();
        return context;
    }

    // Wait before response.
    if (g_cloudBehavior[g_cloudBehaviorIndex].delayBeforeResponseMS > 999)
    {
        sleep((g_cloudBehavior[g_cloudBehaviorIndex].delayBeforeResponseMS + 500) / 1000);
    }
    else
    {
        timespec t;
        t.tv_sec = 0;
        t.tv_nsec = (long)MILLISECONDS_TO_NANOSECONDS(g_cloudBehavior[g_cloudBehaviorIndex].delayBeforeResponseMS);
        timespec remain{};
        int res = nanosleep(&t, &remain);
        if (res == -1)
        {
            switch (errno)
            {
            case EINTR:
                // Interrupted by a signal.
                break;
            case EINVAL:
            case EFAULT:
            {
                sleep(1); // Let's sleep for 1 second.
                break;
            }
            default:
            {
                sleep(1); // Let's sleep for 1 second
                break;
            }
            }
        }
    }
    g_cloudBehaviorIndex++;
    g_cloudBehaviorMutex.unlock();

    // Response with canned http status code.
    g_c2dResponseHandlerFunc(g_cloudBehavior[g_cloudBehaviorIndex - 1].httpStatus, context);
    return context;
};

/**
 * @brief Set the message status, then call the message.statusChangedCallback (if supplied).
 *
 * @param message The message object.
 * @param status  Final message status
 */
void MockSetMessageStatus(ADUC_D2C_Message* message, ADUC_D2C_Message_Status status)
{
    if (message == NULL)
    {
        return;
    }
    message->status = status;
    if (message->statusChangedCallback)
    {
        message->statusChangedCallback(message, status);
    }
}

/**
 * A mock transport function.
*/
int MockMessageTransportFunc(
    void* cloudServiceHandle, void* context, ADUC_C2D_RESPONSE_HANDLER_FUNCTION c2dResponseHandlerFunc)
{
    UNREFERENCED_PARAMETER(cloudServiceHandle);
    INFO("Message transport function called.")
    g_c2dResponseHandlerFunc = c2dResponseHandlerFunc;
    CAPTURE(g_c2dResponseHandlerFunc);
    auto message_processing_context = static_cast<ADUC_D2C_Message_Processing_Context*>(context);
    if (message_processing_context->message.cloudServiceHandle == nullptr
        || *(static_cast<ADUC_ClientHandle*>(message_processing_context->message.cloudServiceHandle)) == nullptr)
    {
        return 1;
    }

    pthread_t transientThread;
    int createResult = pthread_create(&transientThread, nullptr, mock_msg_process_thread_routine, context);

    if (createResult == 0)
    {
        MockSetMessageStatus(&message_processing_context->message, ADUC_D2C_Message_Status_Waiting_For_Response);
    }
    else
    {
        static int nextWait = 30;
        message_processing_context->nextRetryTimeStampEpoch += nextWait;
    }

    return createResult;
}

static void _SetMockCloudBehavior(MockCloudBehavior* b, size_t size, size_t initialIndex, size_t attempts)
{
    UNREFERENCED_PARAMETER(attempts);
    g_cloudBehaviorMutex.lock();
    g_cloudBehavior = b;
    g_cloudBehaviorCount = size;
    g_cloudBehaviorIndex = initialIndex;
    g_attempts = 0;
    g_cloudBehaviorMutex.unlock();
}

bool g_cancelDoWorkThread = false;

void* mock_do_work_thread(void* context)
{
    timespec t;
    t.tv_sec = 0;
    t.tv_nsec = MILLISECONDS_TO_NANOSECONDS(200);
    while (!g_cancelDoWorkThread)
    {
        g_doWorkMutex.lock();
        ADUC_D2C_Messaging_DoWork();
        g_doWorkMutex.unlock();
        nanosleep(&t, nullptr);
    }
    g_cancelDoWorkThread = false;
    return context;
}

static void create_messaging_do_work_thread(void* name)
{
    pthread_t thread;
    pthread_create(&thread, nullptr, mock_do_work_thread, name);
}

static time_t GetTimeSinceEpochInSeconds()
{
    struct timespec timeSinceEpoch
    {
    };
    clock_gettime(CLOCK_REALTIME, &timeSinceEpoch);
    return timeSinceEpoch.tv_sec;
}

void OnMessageProcessCompleted_SaveWholeMessage_And_Signal(void* context, ADUC_D2C_Message_Status status)
{
    UNREFERENCED_PARAMETER(status);
    auto message = static_cast<ADUC_D2C_Message*>(context);
    *static_cast<ADUC_D2C_Message*>(message->userData) = *message;
    // Must signal after done updating global state.
    g_d2cMessageProcessedCond.signal();
};

void OnMessageStatusChanged_SaveWholeMessage_And_Signal(void* context, ADUC_D2C_Message_Status status)
{
    UNREFERENCED_PARAMETER(status);
    auto message = static_cast<ADUC_D2C_Message*>(context);
    *static_cast<ADUC_D2C_Message*>(message->userData) = *message;
    // Must signal after done updating global state.
    g_d2cMessageProcessedCond.signal();
};

static void OnMessageProcessCompleted_SaveStatus(void* context, ADUC_D2C_Message_Status status)
{
    UNREFERENCED_PARAMETER(status);
    auto message = static_cast<ADUC_D2C_Message*>(context);
    *static_cast<ADUC_D2C_Message_Status*>(message->userData) = status;
};

static void OnMessageProcessCompleted_SaveStatus_And_Signal(void* context, ADUC_D2C_Message_Status status)
{
    OnMessageProcessCompleted_SaveStatus(context, status);
    g_d2cMessageProcessedCond.signal();
};

// Make sure that we can deinitialize cleanly while there's a message in-progress.
TEST_CASE("Deinitialization - in progress message", "[.][functional]")
{
    g_testCaseSyncMutex.lock();

    int expectedAttempts = 0;
    const char* message = nullptr;
    auto handle = reinterpret_cast<ADUC_ClientHandle>(-1); // We don't need real handle.

    // Init message processing util, use mock transport, and reduces poll interval to 100ms.
    ADUC_D2C_Messaging_Init();
    ADUC_D2C_Messaging_Set_Transport(ADUC_D2C_Message_Type_Device_Update_Result, MockMessageTransportFunc);

    // Case 1
    expectedAttempts = 0;
    MockCloudBehavior cb1[]{
        { 1000,
          777 }, // Using 777, which is outside or normal http status code. So that we can retry w/o an additional delay.
        { 1000, 777 },
        { 2000, 200 }
    };

    // Ensure that the cloud service is not busy.
    g_cloudServiceMutex.lock();
    _SetMockCloudBehavior(cb1, sizeof(cb1) / sizeof(MockCloudBehavior), 0, 0);
    // Let's the cloud service continue.
    g_cloudServiceMutex.unlock();

    g_doWorkMutex.lock();

    ADUC_D2C_Message resultMessage;
    memset(&resultMessage, 0, sizeof(resultMessage));
    ADUC_D2C_Message_SendAsync(
        ADUC_D2C_Message_Type_Device_Update_Result,
        &handle,
        "Case1 - uninit while message is in-progress.",
        nullptr /* responseCallback */,
        OnMessageProcessCompleted_SaveWholeMessage_And_Signal,
        OnMessageStatusChanged_SaveWholeMessage_And_Signal,
        &resultMessage);

    // Message should be in pending state.
    CHECK(resultMessage.status == ADUC_D2C_Message_Status_Pending);

    // Explicitly call DoWork() once to start processing pending message.
    ADUC_D2C_Messaging_DoWork();

    CHECK(resultMessage.status == ADUC_D2C_Message_Status_Waiting_For_Response);

    // Un-init.
    ADUC_D2C_Messaging_Uninit();

    // Expected 1 attempt since DoWork() already cause the message to send,
    // and state should be 'canceled'
    CHECK(1 == resultMessage.attempts);
    CHECK(resultMessage.status == ADUC_D2C_Message_Status_Canceled);

    g_doWorkMutex.unlock();

    // Done
    g_cloudServiceMutex.unlock();
    g_testCaseSyncMutex.unlock();
}

// Make sure that we can deinitialize cleanly.
TEST_CASE("Deinitialization - pending message", "[.][functional]")
{
    g_testCaseSyncMutex.lock();

    int expectedAttempts = 0;
    const char* message = nullptr;
    auto handle = reinterpret_cast<ADUC_ClientHandle>(-1); // We don't need real handle.

    // Init message processing util, use mock transport, and reduces poll interval to 100ms.
    ADUC_D2C_Messaging_Init();
    ADUC_D2C_Messaging_Set_Transport(ADUC_D2C_Message_Type_Device_Update_Result, MockMessageTransportFunc);

    // Case 1
    message = "Case1 - uninit while message is pending.";
    expectedAttempts = 0;
    MockCloudBehavior cb1[]{ { 2000 /* wait 200ms before response*/, 200 } };

    // Ensure that the cloud service is not busy.
    g_cloudServiceMutex.lock();
    _SetMockCloudBehavior(cb1, sizeof(cb1) / sizeof(MockCloudBehavior), 0, 0);
    // Let's the cloud service continue.
    g_cloudServiceMutex.unlock();

    ADUC_D2C_Message resultMessage;
    memset(&resultMessage, 0, sizeof(resultMessage));
    ADUC_D2C_Message_SendAsync(
        ADUC_D2C_Message_Type_Device_Update_Result,
        &handle,
        message,
        nullptr /* responseCallback */,
        OnMessageProcessCompleted_SaveWholeMessage_And_Signal,
        NULL /* statusChangedCallback */,
        &resultMessage);

    // Message should be in pending state.
    CHECK(resultMessage.status == ADUC_D2C_Message_Status_Pending);

    // Un-init.
    ADUC_D2C_Messaging_Uninit();

    // Expected 0 attempts, and cancel state.
    CHECK(expectedAttempts == resultMessage.attempts);
    CHECK(resultMessage.status == ADUC_D2C_Message_Status_Canceled);

    // Done
    g_testCaseSyncMutex.unlock();
}

TEST_CASE("Simple tests", "[.][functional]")
{
    g_testCaseSyncMutex.lock();

    int expectedAttempts = 0;
    const char* message = nullptr;
    auto handle = reinterpret_cast<ADUC_ClientHandle>(-1); // We don't need real handle.

    // Init message processing util, use mock transport, and reduces poll interval to 100ms.
    ADUC_D2C_Messaging_Init();
    ADUC_D2C_Messaging_Set_Transport(ADUC_D2C_Message_Type_Device_Update_Result, MockMessageTransportFunc);

    create_messaging_do_work_thread((void*)"simple tests");

    // Case 1
    message = "Case 1 - success in 1 attempt";
    expectedAttempts = 1;
    MockCloudBehavior cb1[]{ { 1, 200 } };

    // Ensure that the cloud service is not busy.
    g_cloudServiceMutex.lock();

    _SetMockCloudBehavior(cb1, sizeof(cb1) / sizeof(MockCloudBehavior), 0, 0);

    ADUC_D2C_Message result;
    memset(&result, 0, sizeof(result));
    ADUC_D2C_Message_SendAsync(
        ADUC_D2C_Message_Type_Device_Update_Result,
        &handle,
        message,
        nullptr /* responseCallback */,
        OnMessageProcessCompleted_SaveWholeMessage_And_Signal,
        NULL /* statusChangedCallback */,
        &result);

    // Wait until the message has been processed.
    g_d2cMessageProcessedCond.wait(g_cloudServiceMutex);
    g_cloudServiceMutex.unlock();

    CHECK(expectedAttempts == result.attempts);

    // Case 2
    message = "Case 2 - success in 2 attempts";
    expectedAttempts = 2;
    MockCloudBehavior cb2[]{ { 200, 404 }, { 200, 200 } };

    // Ensure that the cloud service is not busy.
    g_cloudServiceMutex.lock();

    _SetMockCloudBehavior(cb2, sizeof(cb2) / sizeof(MockCloudBehavior), 0, 0);

    memset(&result, 0, sizeof(result));
    ADUC_D2C_Message_SendAsync(
        ADUC_D2C_Message_Type_Device_Update_Result,
        &handle,
        message,
        nullptr /* responseCallback */,
        OnMessageProcessCompleted_SaveWholeMessage_And_Signal,
        NULL /* statusChangedCallback */,
        &result);

    // Wait until the message has been processed.
    g_d2cMessageProcessedCond.wait(g_cloudServiceMutex);
    g_cloudServiceMutex.unlock();

    CHECK(expectedAttempts == result.attempts);

    // Case 3
    message = "Case 3 - success in 4 attempts";
    expectedAttempts = 4;
    MockCloudBehavior cb3[]{ { 100, 403 }, { 100, 404 }, { 100, 403 }, { 100, 200 } };

    // Ensure that the cloud service is not busy.
    g_cloudServiceMutex.lock();

    _SetMockCloudBehavior(cb3, sizeof(cb3) / sizeof(MockCloudBehavior), 0, 0);

    memset(&result, 0, sizeof(result));
    ADUC_D2C_Message_SendAsync(
        ADUC_D2C_Message_Type_Device_Update_Result,
        &handle,
        message,
        nullptr /* responseCallback */,
        OnMessageProcessCompleted_SaveWholeMessage_And_Signal,
        NULL /* statusChangedCallback */,
        &result);

    // Wait until the message has been processed.
    g_d2cMessageProcessedCond.wait(g_cloudServiceMutex);
    g_cloudServiceMutex.unlock();

    CHECK(expectedAttempts == result.attempts);

    // Done
    g_cancelDoWorkThread = true;
    ADUC_D2C_Messaging_Uninit();
    g_testCaseSyncMutex.unlock();
}

TEST_CASE("Bad http status retry info", "[.][functional]")
{
    g_testCaseSyncMutex.lock();

    int expectedAttempts = 0;
    const char* message = nullptr;
    g_attempts = 0;
    auto handle = static_cast<ADUC_ClientHandle>((void*)(1)); // We don't need real handle.

    // Init message processing util, use mock transport, and reduces poll interval to 100ms.
    ADUC_D2C_Messaging_Init();
    ADUC_D2C_Messaging_Set_Transport(ADUC_D2C_Message_Type_Device_Update_Result, MockMessageTransportFunc);

    create_messaging_do_work_thread((void*)"bad retry info");

    // Note: only 1 attempt expected based on g_defaultRetryStrategy_no_calc_func strategy.
    ADUC_D2C_Messaging_Set_Retry_Strategy(
        ADUC_D2C_Message_Type_Device_Update_Result, &g_defaultRetryStrategy_no_calc_func);

    // Case 1
    message = "Case 1 - no retries - missing timestamp calculation proc";
    expectedAttempts = 1;
    MockCloudBehavior cb1[]{ { 100, 555 }, { 100, 200 } };

    // Ensure that the cloud service is not busy.
    g_cloudServiceMutex.lock();

    _SetMockCloudBehavior(cb1, sizeof(cb1) / sizeof(MockCloudBehavior), 0, 0);

    ADUC_D2C_Message result;
    memset(&result, 0, sizeof(result));
    ADUC_D2C_Message_SendAsync(
        ADUC_D2C_Message_Type_Device_Update_Result,
        &handle,
        message,
        nullptr /* responseCallback */,
        OnMessageProcessCompleted_SaveWholeMessage_And_Signal,
        NULL /* statusChangedCallback */,
        &result);

    // Wait until the message has been processed.
    g_d2cMessageProcessedCond.wait(g_cloudServiceMutex);
    g_cloudServiceMutex.unlock();

    CHECK(expectedAttempts == result.attempts);

    // Case 2
    message = "Case 2 - retry needed - missing timestamp calculation proc";
    expectedAttempts = 2;
    MockCloudBehavior cb2[]{ { 100, 601 }, { 100, 200 } };

    // Ensure that the cloud service is not busy.
    g_cloudServiceMutex.lock();

    _SetMockCloudBehavior(cb2, sizeof(cb2) / sizeof(MockCloudBehavior), 0, 0);

    memset(&result, 0, sizeof(result));
    ADUC_D2C_Message_SendAsync(
        ADUC_D2C_Message_Type_Device_Update_Result,
        &handle,
        message,
        nullptr /* responseCallback */,
        OnMessageProcessCompleted_SaveWholeMessage_And_Signal,
        NULL /* statusChangedCallback */,
        &result);

    // Wait until the message has been processed.
    g_d2cMessageProcessedCond.wait(g_cloudServiceMutex);
    g_cloudServiceMutex.unlock();

    CHECK(expectedAttempts == result.attempts);

    // Done
    g_cancelDoWorkThread = true;
    ADUC_D2C_Messaging_Uninit();
    g_testCaseSyncMutex.unlock();
}

// Send message #1 message (service will took 5 seconds to process)
// Wait for 2 seconds to ensure that message# 1 is in progress, then send message #2 and #3 back to back.
// Expected result:
//     msg#1 success
//     msg#2 replaced
//     msg#3 success

TEST_CASE("Message replacement test", "[.][functional]")
{
    g_testCaseSyncMutex.lock();

    const char* message = nullptr;

    ADUC_D2C_Message_Status message1FinalStatus = ADUC_D2C_Message_Status_Pending;
    ADUC_D2C_Message_Status message2FinalStatus = ADUC_D2C_Message_Status_Pending;
    ADUC_D2C_Message_Status message3FinalStatus = ADUC_D2C_Message_Status_Pending;

    auto handle = static_cast<ADUC_ClientHandle>((void*)(1)); // We don't need real handle.

    // Init message processing util, use mock transport, and reduces poll interval to 100ms.
    ADUC_D2C_Messaging_Init();
    ADUC_D2C_Messaging_Set_Transport(ADUC_D2C_Message_Type_Device_Update_Result, MockMessageTransportFunc);

    create_messaging_do_work_thread((void*)"replacement test");

    // Note: speed up the message processing thread.
    ADUC_D2C_Messaging_Set_Retry_Strategy(
        ADUC_D2C_Message_Type_Device_Update_Result, &g_defaultRetryStrategy_fast_speed);

    MockCloudBehavior cb1[]{ { 500, 200 }, { 200, 200 }, { 200, 200 } };

    message = "Message 1";

    // Ensure that the cloud service is not busy.
    g_cloudServiceMutex.lock();

    _SetMockCloudBehavior(cb1, sizeof(cb1) / sizeof(MockCloudBehavior), 0, 0);

    ADUC_D2C_Message_SendAsync(
        ADUC_D2C_Message_Type_Device_Update_Result,
        &handle,
        message,
        nullptr /* responseCallback */,
        OnMessageProcessCompleted_SaveStatus,
        NULL /* statusChangedCallback */,
        &message1FinalStatus);

    // Wait for message 1 to be picked up.
    sleep(2);

    message = "Message 2";
    ADUC_D2C_Message_SendAsync(
        ADUC_D2C_Message_Type_Device_Update_Result,
        &handle,
        message,
        nullptr /* responseCallback */,
        OnMessageProcessCompleted_SaveStatus,
        NULL /* statusChangedCallback */,
        &message2FinalStatus);

    message = "Message 3";
    ADUC_D2C_Message_SendAsync(
        ADUC_D2C_Message_Type_Device_Update_Result,
        &handle,
        message,
        nullptr /* responseCallback */,
        OnMessageProcessCompleted_SaveStatus_And_Signal,
        NULL /* statusChangedCallback */,
        &message3FinalStatus);

    // Wait until the message has been processed.
    g_d2cMessageProcessedCond.wait(g_cloudServiceMutex);
    g_cloudServiceMutex.unlock();

    CHECK(message1FinalStatus == ADUC_D2C_Message_Status_Success);
    CHECK(message2FinalStatus == ADUC_D2C_Message_Status_Replaced);
    CHECK(message3FinalStatus == ADUC_D2C_Message_Status_Success);

    // Done
    g_cancelDoWorkThread = true;
    ADUC_D2C_Messaging_Uninit();
    g_testCaseSyncMutex.unlock();
}

TEST_CASE("30 retries - httpStatus 401", "[.][functional]")
{
    g_testCaseSyncMutex.lock();

    int expectedAttempts = 0;
    const char* message = nullptr;
    auto handle = static_cast<ADUC_ClientHandle>((void*)(1)); // We don't need real handle.

    // Init message processing util, use mock transport, and reduces poll interval to 100ms.
    ADUC_D2C_Messaging_Init();
    ADUC_D2C_Messaging_Set_Transport(ADUC_D2C_Message_Type_Device_Update_Result, MockMessageTransportFunc);

    create_messaging_do_work_thread((void*)"30 retries");

    // Note: speed up the message processing thread.
    ADUC_D2C_Messaging_Set_Retry_Strategy(
        ADUC_D2C_Message_Type_Device_Update_Result, &g_defaultRetryStrategy_fast_speed);

    // Case 1 - received error 29 times, then success.
    // The purpose of this test is to exercise threads synchronization with very small polling and retry times.
    MockCloudBehavior cb1[30];
    expectedAttempts = sizeof(cb1) / sizeof(MockCloudBehavior);

    for (int i = 0; i < expectedAttempts - 1; i++)
    {
        cb1[i].delayBeforeResponseMS = 10;
        cb1[i].httpStatus =
            777; // Using 777, which is outside or normal http status code. So that we can retry w/o an additional delay.
    }
    cb1[expectedAttempts - 1].delayBeforeResponseMS = 5;
    cb1[expectedAttempts - 1].httpStatus = 200;

    message = "Case 1 - 29 error responses, then 1 success response.";

    // Ensure that the cloud service is not busy.
    g_cloudServiceMutex.lock();

    _SetMockCloudBehavior(cb1, sizeof(cb1) / sizeof(MockCloudBehavior), 0, 0);

    ADUC_D2C_Message result;
    memset(&result, 0, sizeof(result));
    ADUC_D2C_Message_SendAsync(
        ADUC_D2C_Message_Type_Device_Update_Result,
        &handle,
        message,
        nullptr /* responseCallback */,
        OnMessageProcessCompleted_SaveWholeMessage_And_Signal,
        nullptr /* statusChangedCallback */,
        &result);

    // Wait until the message has been processed.
    g_d2cMessageProcessedCond.wait(g_cloudServiceMutex);
    g_cloudServiceMutex.unlock();

    CHECK(expectedAttempts == result.attempts);

    // Done
    g_cancelDoWorkThread = true;
    ADUC_D2C_Messaging_Uninit();
    g_testCaseSyncMutex.unlock();
}
