/**
 * @file d2c_messaging.h
 * @brief Utilities for the Device Update Agent Device-to-Cloud messaging.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_D2C_MESSAGING_H
#define ADUC_D2C_MESSAGING_H

#include "aduc/c_utils.h"
#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>

EXTERN_C_BEGIN

#define MILLISECONDS_TO_NANOSECONDS(ms) (ms * 1000000)

/**
 * @brief The message types currently supported by the Device Update Agent.
 */
typedef enum _tagADUC_D2C_Message_Type
{
    ADUC_D2C_Message_Type_Device_Update_Result = 0, /**< deviceUpdate interface reported property */
    ADUC_D2C_Message_Type_Device_Update_ACK,        /**< deviceUpdate interface ACK */
    ADUC_D2C_Message_Type_Device_Information,       /**< deviceInformation interface reported property */
    ADUC_D2C_Message_Type_Diagnostics,              /**< diagnostics interface reported property */
    ADUC_D2C_Message_Type_Diagnostics_ACK,          /**< diagnostics interface ACK */
    ADUC_D2C_Message_Type_Max
} ADUC_D2C_Message_Type;

typedef enum _tagADUC_D2C_Message_Status
{
    ADUC_D2C_Message_Status_Pending = 0,            /**< Waiting to be processed */
    ADUC_D2C_Message_Status_In_Progress,            /**< Being processed by the messages processor */
    ADUC_D2C_Message_Status_Waiting_For_Response,   /**< Sent to the cloud, waiting for response */
    ADUC_D2C_Message_Status_Success,                /**< Message has been processed successfully */
    ADUC_D2C_Message_Status_Failed,                 /**< A failure occurred. No longer process */
    ADUC_D2C_Message_Status_Replaced,               /**< Message was replaced by a new one */
    ADUC_D2C_Message_Status_Canceled,               /**< Message was canceled */
    ADUC_D2C_Message_Status_Max_Retries_Reached,    /**< Maximum number of retries reached */
} ADUC_D2C_Message_Status;

/**
 * A function used for calculating a delay time before the next retry.
 */
typedef time_t (*ADUC_D2C_NEXT_RETRY_TIMESTAMP_CALC_FUNC)(
    int additionalDelaySecs, unsigned int retries, long initialDelayMS, long maxDelaySecs, double maxJitterPercent);

/**
 * @brief The data structure used for deciding how to handle the response from the cloud.
 */
typedef struct _tagADUC_D2C_HttpStatus_Retry_Info
{
    int httpStatusMin;          /**< Indicates minimum boundary of the http status code */
    int httpStatusMax;          /**< Indicates minimum boundary of the http status code */
    int additionalDelaySecs;    /**< An additional wait time before retrying the request */
    ADUC_D2C_NEXT_RETRY_TIMESTAMP_CALC_FUNC
        retryTimestampCalcFunc; /**< A function used to calculate the next retry timestamp */
    unsigned int maxRetry;      /**< Maximum retries */
} ADUC_D2C_HttpStatus_Retry_Info;

/**
 * @brief The data structure that contains information about the retry strategy for each message type.
 */
typedef struct _tagADUC_D2C_RetryStrategy
{
    ADUC_D2C_HttpStatus_Retry_Info*
        httpStatusRetryInfo;            /**< A collection of retry info for each group of of http status codes */
    size_t httpStatusRetryInfoSize;     /**< Size of httpStatusRetryInfo array */
    unsigned int maxRetries;            /**< Maximum number of retries */
    unsigned long maxDelaySecs;         /**< Maximum wait time before retry (in seconds) */
    unsigned long fallbackWaitTimeSec;  /**< The fallback time when regular timestamp calculation failed. */
    unsigned long initialDelayMS;       /**< Backoff factor (in milliseconds ) */
    double maxJitterPercent;            /**< The maximum number of jitter percent (0 - 100)*/
} ADUC_D2C_RetryStrategy;

/**
 * @brief A callback that is called when the device received a response from the cloud.
 * 
 */
typedef bool (*ADUC_D2C_MESSAGE_HTTP_RESPONSE_CALLBACK)(int http_status_code, void* userDataContext);

/**
 * @brief A callback that is called when the message is no longer being processed.
 *        
 *  
 */
typedef void (*ADUC_D2C_MESSAGE_COMPLETED_CALLBACK)(void* message, ADUC_D2C_Message_Status status);

/**
 * @brief A callback that is called when the message status has changed.
*/
typedef void (*ADUC_D2C_MESSAGE_STATUS_CHANGED_CALLBACK)(void* message, ADUC_D2C_Message_Status status);

/**
 * @brief A function that is used for handling a cloud to device response.

 */
typedef void (*ADUC_C2D_RESPONSE_HANDLER_FUNCTION)(int http_status_code, void* userDataContext);

/**
 * @brief A function that is used for sending a message to the cloud.

 */
typedef int (*ADUC_D2C_MESSAGE_TRANSPORT_FUNCTION)(
    void* cloudServiceHandle, void* context, ADUC_C2D_RESPONSE_HANDLER_FUNCTION c2dResponseHandlerFunc);

/**
 * @brief A device to cloud message data structure.
 */
typedef struct _tagADUC_D2C_Message
{
    void* cloudServiceHandle;       /**< The cloud service handle. e.g. ADUC_ClientHandle */
    const char* originalContent;    /**< The original content (message) */
    char* content;                  /**< The copy of the original content (message) */
    time_t contentSubmitTime;       /**< Submit time */
    ADUC_D2C_MESSAGE_HTTP_RESPONSE_CALLBACK responseCallback;       /**< A callback that is called when received a http response from the cloud */
    ADUC_D2C_MESSAGE_COMPLETED_CALLBACK completedCallback;          /**< A callback that is called when the message is no longer being processed */
    ADUC_D2C_MESSAGE_STATUS_CHANGED_CALLBACK statusChangedCallback; /**< A callback that is called when the message status has changed  */
    ADUC_D2C_Message_Status status; /**< The current message status */
    void* userData;                 /**< A data provided by caller */
    int lastHttpStatus;             /**< The latest http status code received for this message */
    unsigned int attempts;          /**< Total number of a send attempts */
} ADUC_D2C_Message;

/**
 * @brief A data structure that contains information about the message processing job.
 */
typedef struct _tagADUC_D2C_Message_Processing_Context
{
    ADUC_D2C_Message_Type type; /**< The property type */

    bool initialized;       /**< Indicates whether this thread context is initialized */
    
    pthread_mutex_t mutex;  /**< Mutex to protect the thread state */

    ADUC_D2C_MESSAGE_TRANSPORT_FUNCTION transportFunc; /**< The function used for sending message to the cloud */

    ADUC_D2C_Message message;       /**< The message data to be send to the cloud service */
    ADUC_D2C_RetryStrategy* retryStrategy; /**< Retry strategy information */
    unsigned int retries;           /**< Number of retries */
    time_t nextRetryTimeStampEpoch; /**< The next retry time stamp. This is the time since epoch, in seconds */
} ADUC_D2C_Message_Processing_Context;

/**
 * @brief Initializes messaging utility.
 *
 * @return Returns true if success.
 */
bool ADUC_D2C_Messaging_Init();

/**
 * @brief Uninitializes messaging utility.
 */
void ADUC_D2C_Messaging_Uninit();

/**
 * @brief Performs messaging processing tasks.
 *
 * Note: must call this function every 100ms - 200ms to ensure that the Device to Cloud messages are processed in timely manner.
 *
 **/
void ADUC_D2C_Messaging_DoWork();

/**
 * @brief Submits the message to messaging utility queue. If the message for specified @p type already exist, it will be replaced by the latest message.
 * 
 *        IMPORTANT: The implementation of @p responseCallback, @p completedCallback, and @p statusChangedCallback MUST NOT
 *        call any ADUC_D2C_* functions. Otherwise, a dead-lock may occurs.
 * 
 * @param type The message type.
 * @param cloudServiceHandle An opaque pointer to the underlying cloud service handle.
 *                           By default, this is the handle to an Azure Iot C PnP device client.
 * @param message The message content. For example, a JSON string containing 'reported' property of the IoT Hub Device Twin.
 * @param responseCallback A callback to be called when the device received a http response. If NULL, the default callback
 *                         will be used, which will stop processing a message if http status code is >= 200 and < 300.
 * @param completedCallback A optional callback to be called when the messages processor stopped processing the message.
 * @param statusChangedCallback A optional callback to be called when the messages status has changed.
 * @param userData An additional user data.
 *
 * @return Returns true if message successfully added to the pending-messages queue.
 */
bool ADUC_D2C_Message_SendAsync(
    ADUC_D2C_Message_Type type,
    void* cloudServiceHandle,
    const char* message,
    ADUC_D2C_MESSAGE_HTTP_RESPONSE_CALLBACK responseCallback,
    ADUC_D2C_MESSAGE_COMPLETED_CALLBACK completedCallback,
    ADUC_D2C_MESSAGE_STATUS_CHANGED_CALLBACK statusChangedCallback,
    void* userData);

/**
 * @brief Sets the messaging transport. By default, the messaging utility will send messages to IoT Hub.
 *
 *        IMPORTANT: The implementation of @p transportFunc MUST NOT call any ADUC_D2C_* functions. 
 *         Otherwise, a dead-lock may occurs.
 * 
 * @param type The message type.
 * @param transportFunc The message transport function.
 */
void ADUC_D2C_Messaging_Set_Transport(ADUC_D2C_Message_Type type, ADUC_D2C_MESSAGE_TRANSPORT_FUNCTION transportFunc);

/**
 * @brief Sets the retry strategy for the specified @p type
 *
 * @param type The message type.
 * @param strategy The retry strategy information.
 */
void ADUC_D2C_Messaging_Set_Retry_Strategy(ADUC_D2C_Message_Type type, ADUC_D2C_RetryStrategy* strategy);

/**
 * @brief The default message transport function.
 *
 *        IMPORTANT: The implementation of @p c2dResponseHandlerFunc MUST NOT call any ADUC_D2C_* functions.
 *        Otherwise, a dead-lock may occurs.
 * 
 * @param cloudServiceHandle A pointer to the cloud service handle.
 * @param context The D2C messaging context.
 * @param c2dResponseHandlerFunc The D2C response handler.
 * @return int
 */
int ADUC_D2C_Default_Message_Transport_Function(
    void* cloudServiceHandle, void* context, ADUC_C2D_RESPONSE_HANDLER_FUNCTION c2dResponseHandlerFunc);

/**
 * @brief A default retry delay calculator function. 
 * 
 * @param additionalDelaySecs Additional delay time, in seconds.
 * @param retries A current retries count.
 * @param initialDelayMS An initial delay time that is used in the calculation function, in milliseconds.     
 * @param maxDelaySecs  A max delay time, in seconds.
 * @param maxJitterPercent A maximum jitter percentage.
 * @return time_t Return a timestamp (since epoch) for the next retry.
 */
time_t ADUC_D2C_RetryDelayCalculator(int additionalDelaySecs, unsigned int retries, long initialDelayMS, long maxDelaySecs, double maxJitterPercent);


EXTERN_C_END

#endif // ADUC_D2C_MESSAGING_H
