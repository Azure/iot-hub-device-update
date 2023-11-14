#include "aduc/topic_mgmt_lifecycle.h"
#include "aduc/adu_enrollment_management.h"
#include "aduc/enrollment_request_operation.h" // CreateAndInitializeEnrollmentRequestOperation
#include "aduc/agent_module_interface_internal.h" // Module interface function declarations for every per-topic module
#include <aduc/retry_utils.h> // ADUC_Retriable_Operation_Context

/**
 * @brief Creates an entry in the array of LifeCycleFn structs by concatenating
 * to create the necessary tokens. Must stay in sync with ADUC_AGENT_MODULE_INTERFACE.
 * @details The token concatenations here must be in sync with EXPAND_MODULE_INTERFACE_DECLS
 * and the ADUC_AGENT_MODULE_INTERFACE structure.
 */
#define EXPAND_AGENT_MODULE_INTERFACE_FUNCTION_SET(MODULE_NAME) \
    {                                                           \
        CreateAndInitialize ## MODULE_NAME ## RequestOperation, \
        ADUC_ ## MODULE_NAME ## _Management_GetContractInfo,    \
        ADUC_ ## MODULE_NAME ## _Management_Initialize,         \
        ADUC_ ## MODULE_NAME ## _Management_Deinitialize,       \
        ADUC_ ## MODULE_NAME ## _Management_DoWork,             \
        ADUC_ ## MODULE_NAME ## _Management_Destroy,            \
    }

// clang-format off

/**
 * @brief The sets of lifecycle functions for ADUC_AGENT_MODULE_INTERFACE instancing.
 * @details This struct's elements must stay in sync with TOPIC_MGMT_MODULE enum in topic_mgmt_lifecycle.h
 */
struct
{
    ADUC_AGENT_MODULE_GET_CONTRACT_INFO contractInfoFn;
    ADUC_AGENT_MODULE_INITIALIZE initializeFn;
    ADUC_AGENT_MODULE_DEINITIALIZE deinitializeFn;
    ADUC_AGENT_MODULE_DO_WORK doWorkFn;
    ADUC_AGENT_MODULE_DESTROY destroyFn;
} LifeCycleFn[] = {
    EXPAND_AGENT_MODULE_INTERFACE_FUNCTION_SET(Enrollment), // TOPIC_MGMT_MODULE_Enrollment
    EXPAND_AGENT_MODULE_INTERFACE_FUNCTION_SET(AgentInfo),  // TOPIC_MGMT_MODULE_AgentInfo
    // EXPAND_AGENT_MODULE_INTERFACE_FUNCTION_SET(Update),  // TOPIC_MGMT_MODULE_Update
    // EXPAND_AGENT_MODULE_INTERFACE_FUNCTION_SET(UpdateResults), // TOPIC_MGMT_MODULE_UpdateResults
};

// clang-format on

//
// Public interface
//

ADUC_AGENT_MODULE_HANDLE TopicMgmtLifecycle_Create(TOPIC_MGMT_MODULE modTopic)
{
    ADUC_AGENT_MODULE_INTERFACE* result = NULL;
    ADUC_AGENT_MODULE_INTERFACE* interface = NULL;

    ADUC_Retriable_Operation_Context* operationContext = CreateAndInitializeEnrollmentRequestOperation();
    if (operationContext == NULL)
    {
        Log_Error("Failed to create enrollment request operation");
        goto done;
    }

    ADUC_ALLOC(interface);

    interface->getContractInfo = LifeCycleFn[modTopic].contractInfoFn;
    interface->initializeModule = LifeCycleFn[modTopic].initializeFn;
    interface->deinitializeModule = LifeCycleFn[modTopic].deinitializeFn;
    interface->doWork = LifeCycleFn[modTopic].doWorkFn;
    interface->destroy = LifeCycleFn[modTopic].destroyFn;
    interface->moduleData = operationContext;
    operationContext = NULL; // transfer ownership

    result = interface;
    interface = NULL; // transfer ownership

done:

    if (operationContext != NULL)
    {
        operationContext->dataDestroyFunc(operationContext);
        operationContext->operationDestroyFunc(operationContext);
        free(operationContext);
    }

    free(interface);

    return (ADUC_AGENT_MODULE_HANDLE)(result);
}

void TopicMgmtLifecycle_Destroy(ADUC_AGENT_MODULE_HANDLE handle)
{

}
