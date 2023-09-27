/**
 * @file device_update_workflow_module.c
 * @brief Implementation for the Device Update Workflow Module.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adu_core_interface.h"
#include "aduc/adu_types.h"
#include "aduc/config_utils.h"
#include "aduc/d2c_messaging.h"
#include "aduc/extension_manager.h"
#include "aduc/logging.h"
#include "aduc/types/workflow.h"
#include "du_agent_sdk/agent_module_interface.h"
#include <stdlib.h>

ADUC_AGENT_CONTRACT_INFO g_iotHubClientContractInfo = {
    "Microsoft",
    "Device Update Workflow Module",
    1,
    "Microsoft/DeviceUpdateWorkflowModule:1"
};

/**
 * @brief Gets the extension contract info.
 * @param handle The handle to the module. This is the same handle that was returned by the Create function.
 * @return ADUC_AGENT_CONTRACT_INFO The extension contract info.
 */
const ADUC_AGENT_CONTRACT_INFO* DeviceUpdateWorkflowModule_GetContractInfo(ADUC_AGENT_MODULE_HANDLE handle)
{
    IGNORED_PARAMETER(handle);
    return &g_iotHubClientContractInfo;
}

/**
 * @brief Initialize the IoTHub Client module.
*/
int DeviceUpdateWorkflowModule_Initialize(ADUC_AGENT_MODULE_HANDLE handle, void* moduleInitData)
{
    IGNORED_PARAMETER(handle);
    IGNORED_PARAMETER(moduleInitData);

    ADUC_Result result = { ADUC_GeneralResult_Failure };

    ADUC_Logging_Init(0, "device-update-workflow-module");

    if (!ADUC_D2C_Messaging_Init())
    {
        goto done;
    }

    result.ResultCode = ADUC_GeneralResult_Success;
    result.ExtendedResultCode = 0;

done:
    return result.ResultCode == ADUC_GeneralResult_Success ? 0 : -1;
}

/**
 * @brief Deinitialize the IoTHub Client module.
 *
 * @param module The handle to the module. This is the same handle that was returned by the Create function.
 *
 * @return int 0 on success.
 */
int DeviceUpdateWorkflowModule_Deinitialize(ADUC_AGENT_MODULE_HANDLE module)
{
    Log_Info("Deinitialize");
    ADUC_D2C_Messaging_Uninit();
    ADUC_Logging_Uninit();
    return 0;
}

/*
 * @brief Destroy the Device Update Agent Module for IoT Hub PnP Client.
 * @param handle The handle to the module. This is the same handle that was returned by the Create function.
 */
void DeviceUpdateWorkflowModule_Destroy(ADUC_AGENT_MODULE_HANDLE handle)
{
    ADUC_AGENT_MODULE_INTERFACE* interface = (ADUC_AGENT_MODULE_INTERFACE*)handle;
    AzureDeviceUpdateCoreInterface_Destroy(&interface->moduleData);
    free(interface);
}

/**
 * @brief Perform the work for the extension. This must be a non-blocking operation.
 *
 * @return ADUC_Result
 */
int DeviceUpdateWorkflowModule_DoWork(ADUC_AGENT_MODULE_HANDLE handle)
{
    ADUC_AGENT_MODULE_INTERFACE* interface = (ADUC_AGENT_MODULE_INTERFACE*)handle;
    AzureDeviceUpdateCoreInterface_DoWork(interface->moduleData);
    ADUC_D2C_Messaging_DoWork();
    return 0;
}

/**
 * @brief Create a Device Update Agent Module for IoT Hub PnP Client.
 *
 * @return ADUC_AGENT_MODULE_HANDLE The handle to the module.
 */
ADUC_AGENT_MODULE_HANDLE DeviceUpdateWorkflowModule_Create()
{
    bool ret = false;
    ADUC_WorkflowData *workflowData =  NULL;
    ADUC_AGENT_MODULE_HANDLE handle = NULL;
    ADUC_AGENT_MODULE_INTERFACE* interface = malloc(sizeof(ADUC_AGENT_MODULE_INTERFACE));
    if (interface == NULL)
    {
        Log_Error("Out of memory");
        goto done;
    }

    // TODO (nox-msft) - pass in launchArgs
    ret = AzureDeviceUpdateCoreInterface_Create((void**)&workflowData, 0, NULL);
    if (!ret)
    {
        goto done;
    }

    workflowData->CommunicationChannel = ADUC_CommunicationChannelType_MQTTBroker;
    interface->moduleData = workflowData;
    interface->destroy = DeviceUpdateWorkflowModule_Destroy;
    interface->getContractInfo = DeviceUpdateWorkflowModule_GetContractInfo;
    interface->doWork = DeviceUpdateWorkflowModule_DoWork;
    interface->initializeModule = DeviceUpdateWorkflowModule_Initialize;
    interface->deinitializeModule = DeviceUpdateWorkflowModule_Deinitialize;

    handle = (ADUC_AGENT_MODULE_HANDLE)interface;

done:
    if (handle == NULL)
    {
        free(interface);
    }
    return handle;
}

