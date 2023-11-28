
#include "aduc/agent_module_interface_internal.h"

#include <aduc/adu_mqtt_common.h> // OperationContextFromAgentModuleHandle
#include <aduc/logging.h>
#include <aduc/retry_utils.h> // ADUC_Retriable_Operation_Context
#include <du_agent_sdk/agent_common.h> // IGNORED_PARAMETER

/////////////////////////////////////////////////////////////////////////////
//
// BEGIN - AGENT_MODULE_INTERFACE_INTERNAL_H interface implementation
//

/**
 * @brief Gets the extension contract info.
 * @param handle The handle to the module. This is the same handle that was returned by the Create function.
 * @return ADUC_AGENT_CONTRACT_INFO The extension contract info.
 */
const ADUC_AGENT_CONTRACT_INFO* ADUC_AgentInfo_Management_GetContractInfo(ADUC_AGENT_MODULE_HANDLE handle)
{
    static ADUC_AGENT_CONTRACT_INFO s_moduleContractInfo = {
        "Microsoft", "Device Update AgentInfo Module", 1, "Microsoft/DUAgentInfoModule:1"
    };

    IGNORED_PARAMETER(handle);
    return &s_moduleContractInfo;
}

/**
 * @brief Initialize the agentinfo management.
 * @param handle The agent module handle.
 * @param initData The initialization data.
 * @return 0 on successful initialization; -1, otherwise.
 */
int ADUC_AgentInfo_Management_Initialize(ADUC_AGENT_MODULE_HANDLE handle, void* initData)
{
    IGNORED_PARAMETER(initData);
    ADUC_Retriable_Operation_Context* context = OperationContextFromAgentModuleHandle(handle);
    if (context == NULL)
    {
        Log_Error("Failed to get operation context");
        return -1;
    }

    return 0;
}

/**
 * @brief Deinitialize the agentinfo management.
 * @param handle The module handle.
 * @return int 0 on success.
 */
int ADUC_AgentInfo_Management_Deinitialize(ADUC_AGENT_MODULE_HANDLE handle)
{
    ADUC_Retriable_Operation_Context* context = OperationContextFromAgentModuleHandle(handle);
    if (context == NULL)
    {
        Log_Error("Failed to get operation context");
        return false;
    }
    context->cancelFunc(context);
    return 0;
}

/**
 * @brief AgentInfo mangement do work function.
 *
 * @param handle The module handle.
 * @return int 0 on success.
 */
int ADUC_AgentInfo_Management_DoWork(ADUC_AGENT_MODULE_HANDLE handle)
{
    ADUC_Retriable_Operation_Context* context = OperationContextFromAgentModuleHandle(handle);
    if (context == NULL)
    {
        Log_Error("Failed to get operation context");
        return -1;
    }

    context->doWorkFunc(context);

    return 0;
}

//
// END - AGENT_MODULE_INTERFACE_INTERNAL_H implementation
//
/////////////////////////////////////////////////////////////////////////////
