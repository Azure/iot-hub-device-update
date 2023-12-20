#include "aduc/adu_upd_utils.h"
#include <aduc/adu_module_state.h> // ADUC_MQTT_CLIENT_MODULE_STATE
#include <aduc/adu_mqtt_protocol.h> // ADU_RESPONSE_MESSAGE_RESULT_CODE_*
#include <aduc/agent_state_store.h>
#include <aduc/logging.h>
#include <aduc/mqtt_broker_common.h> // ADUC_MQTT_Message_Context
#include <aduc/string_c_utils.h> // ADUC_Safe_StrCopyN
#include <aduc/workqueue.h> // WorkQueueHandle
#include <string.h> // strlen

// keep this last to avoid interfering with system headers
#include <aduc/aduc_banned.h>

/**
 * @brief Gets the update data object from the agent info operation context.
 * @param context The update operation context.
 * @return ADUC_Update_Request_Operation_Data* The update data object.
 */
ADUC_Update_Request_Operation_Data* UpdateData_FromOperationContext(ADUC_Retriable_Operation_Context* context)
{
    if (context == NULL || context->data == NULL)
    {
        Log_Error("Null input (context:%p, data:%p)", context, context->data);
        return NULL;
    }

    return (ADUC_Update_Request_Operation_Data*)(context->data);
}

/**
 * @brief Gets the update workqueue handle from the callback user object.
 * @param obj The callback's user object.
 * @return ADUC_Retriable_Operation_Context* The retriable operation context.
 */
WorkQueueHandle WorkQueueHandleFromCallbackUserObj(void* obj)
{
    ADUC_MQTT_CLIENT_MODULE_STATE* ownerModuleState = NULL;
    ADUC_WorkQueues* workQueues = NULL;

    ownerModuleState = (ADUC_MQTT_CLIENT_MODULE_STATE*)obj;
    if (ownerModuleState == NULL)
    {
        return NULL;
    }

    workQueues = (ADUC_WorkQueues*)(ownerModuleState->moduleInitData);
    if (workQueues == NULL)
    {
        return NULL;
    }

    return workQueues->updateWorkQueue;
}

/**
 * @brief Gets the reporting workqueue handle from the callback user object.
 * @param obj The callback's user object.
 * @return ADUC_Retriable_Operation_Context* The retriable operation context.
 */
WorkQueueHandle ReportingWorkQueueHandleFromCallbackUserObj(void* obj)
{
    ADUC_MQTT_CLIENT_MODULE_STATE* ownerModuleState = (ADUC_MQTT_CLIENT_MODULE_STATE*)obj;
    ADUC_WorkQueues* workQueues = (ADUC_WorkQueues*)(ownerModuleState->moduleInitData);
    return workQueues->reportingWorkQueue;
}

/**
 * @brief Gets the retriable operation context from the callback user object.
 * @param obj The callback's user object.
 * @return ADUC_Retriable_Operation_Context* The retriable operation context.
 */
ADUC_Retriable_Operation_Context* RetriableOperationContextFromCallbackUserObj(void* obj)
{
    ADUC_MQTT_CLIENT_MODULE_STATE* ownerModuleState = (ADUC_MQTT_CLIENT_MODULE_STATE*)obj;
    ADUC_AGENT_MODULE_INTERFACE* updateModuleInterface =
        (ADUC_AGENT_MODULE_INTERFACE*)(ownerModuleState->updateModule);
    return (ADUC_Retriable_Operation_Context*)(updateModuleInterface->moduleData);
}

/**
 * @brief Gets the Update Request Operation Data from RetriableOperatioContext.
 * @param retriableOperationContext The retriable operation context.
 * @return ADUC_Enrollment_Request_Operation_Data* The update request operation data.
 */
ADUC_Update_Request_Operation_Data*
UpdateDataFromRetriableOperationContext(ADUC_Retriable_Operation_Context* retriableOperationContext)
{
    if (retriableOperationContext == NULL)
    {
        return NULL;
    }

    return (ADUC_Update_Request_Operation_Data*)(retriableOperationContext->data);
}

/**
 * @brief Sets the correlation id for the update request message.
 * @param updateData The update data object.
 * @param correlationId The correlation id to set.
 */
bool UpdateData_SetCorrelationId(ADUC_Update_Request_Operation_Data* updateData, const char* correlationId)
{
    if (updateData == NULL || correlationId == NULL)
    {
        return false;
    }

    if (!ADUC_Safe_StrCopyN(
            updateData->updReqMessageContext.correlationId,
            ARRAY_SIZE(updateData->updReqMessageContext.correlationId),
            correlationId,
            strlen(correlationId)))
    {
        Log_Error("copy failed");
        return false;
    }

    return true;
}

void AduUpdUtils_TransitionState(ADU_UPD_STATE newState, ADUC_Update_Request_Operation_Data* updateData)
{
    Log_Info(
        "Transition from %d ('%s') to %d ('%s')",
        updateData->updState,
        AduUpdState_str(updateData->updState),
        newState,
        AduUpdState_str(newState));
    updateData->updState = newState;
}
