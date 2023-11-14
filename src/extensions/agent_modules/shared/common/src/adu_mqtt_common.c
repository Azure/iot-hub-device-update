
#include "aduc/adu_mqtt_common.h"

#include <aduc/adu_communication_channel.h> // ADUC_Communication_Channel_MQTT_Subscribe
#include <aduc/logging.h>
#include <du_agent_sdk/agent_module_interface.h> // ADUC_AGENT_MODULE_HANDLE

/**
 * @brief Gets operation context from module handle.
 *
 * @param handle The module handle.
 * @return ADUC_Retriable_Operation_Context* The operation context.
 */
ADUC_Retriable_Operation_Context* operationContextFromAgentModuleHandle(ADUC_AGENT_MODULE_HANDLE handle)
{
    ADUC_AGENT_MODULE_INTERFACE* moduleInterface = (ADUC_AGENT_MODULE_INTERFACE*)handle;
    if (moduleInterface == NULL || moduleInterface->moduleData == NULL)
    {
        return NULL;
    }

    return (ADUC_Retriable_Operation_Context*)moduleInterface->moduleData;
}

/**
 * @brief Sets up the MQTT adu publish and response topics on the message context.
 *
 * @param context The operation context.
 * @param messageContext The message context.
 * @param isScoped Whether to include the scopeId in the publish and response topics.
 * @return false when no topic setup was needed.
 * @return true when failed to setup topics
 */
bool MqttTopicSetupNeeded(ADUC_Retriable_Operation_Context* context, ADUC_MQTT_Message_Context* messageContext, bool isScoped)
{
    const char* scopeId = isScoped ? ADUC_StateStore_GetDeviceUpdateServiceInstance() : NULL;

    // prepare a topic for the request
    if (IsNullOrEmpty(messageContext->publishTopic))
    {
        const char* externalDeviceId = ADUC_StateStore_GetExternalDeviceId();

        if (isScoped)
        {
            messageContext->publishTopic = ADUC_StringFormat(PUBLISH_TOPIC_TEMPLATE_ADU_OTO_WITH_DU_INSTANCE, externalDeviceId, scopeId);
        }
        else
        {
            messageContext->publishTopic = ADUC_StringFormat(PUBLISH_TOPIC_TEMPLATE_ADU_OTO, externalDeviceId);
        }

        if (messageContext->publishTopic == NULL)
        {
            Log_Error("Failed alloc for publish topic. Cancelling operation");
            context->cancelFunc(context);
            return true;
        }

        Log_Info("Set publish topic (scoped: %d): %s", isScoped, messageContext->publishTopic);
    }

    // prepare a topic for the response subscription.
    if (IsNullOrEmpty(messageContext->responseTopic))
    {
        const char* externalDeviceId = ADUC_StateStore_GetExternalDeviceId();

        if (isScoped)
        {
            messageContext->responseTopic = ADUC_StringFormat(SUBSCRIBE_TOPIC_TEMPLATE_ADU_OTO_WITH_DU_INSTANCE, externalDeviceId, scopeId);
        }
        else
        {
            messageContext->responseTopic = ADUC_StringFormat(SUBSCRIBE_TOPIC_TEMPLATE_ADU_OTO, externalDeviceId);
        }

        if (messageContext->responseTopic == NULL)
        {
            Log_Error("failed to allocate memory for response topic. cancelling the operation.");
            context->cancelFunc(context);
            return true;
        }

        Log_Info("Set response topic (scoped: %d): %s", isScoped, messageContext->responseTopic);
    }

    return false;
}

/**
 * @brief Ensures communication channel is setup.
 *
 * @param context
 * @return true if unable to setup the communication channel and retry was attempted.
 * @return false if communication channel is setup.
 */
static bool CommunicationChannelNeededSetup(ADUC_Retriable_Operation_Context* context)
{
    // this operation is depending on the "duservicecommunicationchannel".
    // note: by default, the du service communication already subscribed to the common service-to-device messaging topic.
    if (context->commChannelHandle == NULL)
    {
        context->commChannelHandle =
            ADUC_StateStore_GetCommunicationChannelHandle(ADUC_DU_SERVICE_COMMUNICATION_CHANNEL_ID);
    }

    if (context->commChannelHandle == NULL)
    {
        Log_Info("communication channel is not ready. will retry");
        context->retryFunc(context, &context->retryParams[ADUC_RETRY_PARAMS_INDEX_DEFAULT]);
        return true;
    }

    return false;
}

/**
 * @brief Checks that external device id has been setup and invokes retry if it has not.
 *
 * @param context The retriable operation context.
 * @return true If external device id setup was needed.
 */
bool ExternalDeviceIdSetupNeeded(ADUC_Retriable_Operation_Context* context)
{
    // and ensure that we have a valid external device id. this should usually provided by a dps.
    const char* externalDeviceId = ADUC_StateStore_GetExternalDeviceId();
    if (IsNullOrEmpty(externalDeviceId))
    {
        Log_Info("an external device id is not available. will retry");
        context->retryFunc(context, &context->retryParams[ADUC_RETRY_PARAMS_INDEX_DEFAULT]);
        return true;
    }

    return false;
}

//
// adu_mqtt_common.h public interface
//

/**
 * @brief Sets up all the pre-requisities to do an ADU MQTT topic request, such as comm channel, external id, mqtt topic setup, and subscribing to the topic for the response.
 *
 * @param context The retriable operation context.
 * @param messageContext The message context.
 * @return true if any setup was needed.
 * @details May invoke the retry func if devices is not registered.
 */
bool SettingUpAduMqttRequestPrerequisites(ADUC_Retriable_Operation_Context* context, ADUC_MQTT_Message_Context* messageContext)
{
    if (!ADUC_StateStore_GetIsDeviceRegistered())
    {
        Log_Info("device is not registered. will retry");
        context->retryFunc(context, &context->retryParams[ADUC_RETRY_PARAMS_INDEX_DEFAULT]);
        return true;
    }

    return CommunicationChannelNeededSetup(context)
        || ExternalDeviceIdSetupNeeded(context)
        || MqttTopicSetupNeeded(context, messageContext)
        || (!ADUC_MQTT_COMMON_Ensure_Subscribed_for_Response(context, messageContext));
}

/**
 * @brief Subscribes to the message context's mqtt topic if not already subscribed.
 *
 * @param context The operation context.
 * @param messageContext The message context.
 * @return true On successful subscribe, or if already subscribed.
 */
bool ADUC_MQTT_COMMON_Ensure_Subscribed_for_Response(const ADUC_Retriable_Operation_Context* context, ADUC_MQTT_Message_Context* messageContext)
{
    bool succeeded = false;

    if (!ADUC_Communication_Channel_MQTT_IsSubscribed(
            (ADUC_AGENT_MODULE_HANDLE)context->commChannelHandle, messageContext->responseTopic))
    {
        // Subscribe to the response topic.
        int subscribeMessageId = 0;
        int mqtt_res = ADUC_Communication_Channel_MQTT_Subscribe(
            (ADUC_AGENT_MODULE_HANDLE)context->commChannelHandle,
            messageContext->responseTopic,
            &subscribeMessageId,
            messageContext->qos,
            0 /*options*/,
            NULL /*props*/,
            NULL /*userData*/,
            NULL /*callback*/);

        if (mqtt_res != MOSQ_ERR_SUCCESS)
        {
            Log_Error("Failed to subscribe to response topic. Cancelling the operation.");
            context->retryFunc((ADUC_Retriable_Operation_Context*)context, &context->retryParams[ADUC_RETRY_PARAMS_INDEX_CLIENT_TRANSIENT]);
            goto done;
        }

        Log_Info(
            "Subscribing to response topic '%s', messageId:%d", messageContext->responseTopic, subscribeMessageId);
    }

    succeeded = true;

done:
    return succeeded;

}
