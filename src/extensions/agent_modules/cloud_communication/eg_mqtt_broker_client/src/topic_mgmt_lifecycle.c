#include "aduc/topic_mgmt_lifecycle.h"
#include "aduc/adu_enrollment_management.h"
#include "aduc/agentinfo_request_operation.h" // CreateAndInitializeAgentInfoRequestOperation
#include "aduc/enrollment_request_operation.h" // CreateAndInitializeEnrollmentRequestOperation
#include "aduc/agent_module_interface_internal.h" // Module interface function declarations for every per-topic module
#include <aduc/retry_utils.h> // ADUC_Retriable_Operation_Context

typedef ADUC_Retriable_Operation_Context* (*ADUC_CREATE_AND_INIT_REQUEST_OP_FUNC)();

/**
 * @brief The sets of lifecycle functions for ADUC_AGENT_MODULE_INTERFACE instancing.
 * @details This struct's elements must stay in sync with TOPIC_MGMT_MODULE enum in topic_mgmt_lifecycle.h
 */
struct
{
    ADUC_CREATE_AND_INIT_REQUEST_OP_FUNC createAndInitRequestOperationFn;
    ADUC_AGENT_MODULE_GET_CONTRACT_INFO contractInfoFn;
    ADUC_AGENT_MODULE_INITIALIZE initializeFn;
    ADUC_AGENT_MODULE_DEINITIALIZE deinitializeFn;
    ADUC_AGENT_MODULE_DO_WORK doWorkFn;
    ADUC_AGENT_MODULE_DESTROY destroyFn;
} LifeCycleDelegateSets[] = {
    {
        CreateAndInitializeEnrollmentRequestOperation,
        ADUC_Enrollment_Management_GetContractInfo,
        ADUC_Enrollment_Management_Initialize,
        ADUC_Enrollment_Management_Deinitialize,
        ADUC_Enrollment_Management_DoWork,
        ADUC_Enrollment_Management_Destroy,
    },
    {
        CreateAndInitializeAgentInfoRequestOperation,
        ADUC_AgentInfo_Management_GetContractInfo,
        ADUC_AgentInfo_Management_Initialize,
        ADUC_AgentInfo_Management_Deinitialize,
        ADUC_AgentInfo_Management_DoWork,
        ADUC_AgentInfo_Management_Destroy,
    },
    // {
    //     CreateAndInitializeUpdateRequestOperation,
    //     ADUC_Enrollment_Management_GetContractInfo,
    //     ADUC_Update_Management_Initialize,
    //     ADUC_Update_Management_Deinitialize,
    //     ADUC_Update_Management_DoWork,
    //     ADUC_Update_Management_Destroy,
    // },
    // {
    //     CreateAndInitializeUpdateResultsRequestOperation,
    //     ADUC_UpdateResults_Management_GetContractInfo,
    //     ADUC_UpdateResults_Management_Initialize,
    //     ADUC_UpdateResults_Management_Deinitialize,
    //     ADUC_UpdateResults_Management_DoWork,
    //     ADUC_UpdateResults_Management_Destroy,
    // },
};

// clang-format on

//
// Public interface
//

/**
 * @brief Creates the per-topic Module Handle for a given topic management module.
 *
 * @param modTopic The module topic.
 * @return ADUC_AGENT_MODULE_HANDLE The module handle.
 */
ADUC_AGENT_MODULE_HANDLE TopicMgmtLifecycle_Create(TOPIC_MGMT_MODULE modTopic)
{
    ADUC_AGENT_MODULE_HANDLE handle = NULL;

    ADUC_AGENT_MODULE_INTERFACE* tmp = NULL;

    ADUC_CREATE_AND_INIT_REQUEST_OP_FUNC create_and_init_fn = LifeCycleDelegateSets[modTopic].createAndInitRequestOperationFn;
    ADUC_Retriable_Operation_Context* operationContext = create_and_init_fn();
    if (operationContext == NULL)
    {
        goto done;
    }

    tmp = calloc(1, sizeof(*tmp));
    if (tmp == NULL)
    {
        goto done;
    }

    tmp->getContractInfo = LifeCycleDelegateSets[modTopic].contractInfoFn;
    tmp->initializeModule = LifeCycleDelegateSets[modTopic].initializeFn;
    tmp->deinitializeModule = LifeCycleDelegateSets[modTopic].deinitializeFn;
    tmp->doWork = LifeCycleDelegateSets[modTopic].doWorkFn;
    tmp->destroy = LifeCycleDelegateSets[modTopic].destroyFn;

    tmp->moduleData = operationContext;
    operationContext = NULL;

    handle = tmp;
    tmp = NULL;

done:

    TopicMgmtLifecycle_UninitRetriableOperationContext(operationContext);
    free(operationContext);

    free(tmp);

    return handle;
}

/**
 * @brief  Uninitializes the retriable operation context for management of request operations.
 *
 * @param operationContext The operation context.
 */
void TopicMgmtLifecycle_UninitRetriableOperationContext(ADUC_Retriable_Operation_Context* operationContext)
{
    if (operationContext != NULL)
    {
        operationContext->dataDestroyFunc(operationContext);
        operationContext->operationDestroyFunc(operationContext);
    }
}

// TODO: Figure out where to call TopicMgmtLifecycle_Destroy
void TopicMgmtLifecycle_Destroy(ADUC_AGENT_MODULE_HANDLE handle)
{
    ADUC_AGENT_MODULE_INTERFACE* mod_handle = (ADUC_AGENT_MODULE_INTERFACE*)handle;
    if (mod_handle != NULL)
    {
        // TODO: free moduleData
        // free(mod_handle->moduleData);
        free(mod_handle);
    }
}
