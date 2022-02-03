/**
 * @file adu_core_interface.h
 * @brief Methods to communicate with "urn:azureiot:AzureDeviceUpdateCore:1" interface.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_ADU_CORE_INTERFACE_H
#define ADUC_ADU_CORE_INTERFACE_H

#include <aduc/adu_core_json.h> // ADUCITF_State
#include <aduc/c_utils.h>
#include <aduc/client_handle.h>
#include <aduc/result.h> // ADUC_Result
#include <aduc/types/workflow.h>
#include <azureiot/iothub_client_core_common.h>
#include <stdbool.h>

EXTERN_C_BEGIN

struct tagADUC_UpdateId;
typedef void* ADUC_WorkflowHandle;

//
// Registration/Unregistration
//

/**
 * @brief Handle for communication to service.
 */
extern ADUC_ClientHandle g_iotHubClientHandleForADUComponent;

/**
 * @brief Initialize the interface.
 *
 * @param context Optional context object.
 * @param argc Count of arguments in @p argv
 * @param argv Command line parameters.
 * @return _Bool True on success.
 */
_Bool AzureDeviceUpdateCoreInterface_Create(void** context, int argc, char** argv);

/**
 * @brief Called after the device connected to IoT Hub (device client handler is valid).
 *
 * @param componentContext Context object from Create.
 */
void AzureDeviceUpdateCoreInterface_Connected(void* componentContext);

/**
 * @brief Called regularly after the device connected to the IoT Hub.
 *
 * This allows an interface implementation to do work in a cooperative multitasking environment.
 *
 * @param componentContext Context object from Create.
 */
void AzureDeviceUpdateCoreInterface_DoWork(void* componentContext);

/**
 * @brief Uninitialize the component.
 *
 * @param componentContext Context object from Create function.
 */
void AzureDeviceUpdateCoreInterface_Destroy(void** componentContext);

/**
 * @brief A callback for an 'deviceUpdate' component's property update events.
 */
void AzureDeviceUpdateCoreInterface_PropertyUpdateCallback(
    ADUC_ClientHandle clientHandle, const char* propertyName, JSON_Value* propertyValue, int version, void* context);

//
// Reporting
//

/**
 * @brief Report a new state to the server.
 *
 * @param workflowDataToken A pointer to workflow data object.
 * @param updateState State to report.
 * @param result Result to report (optional, can be NULL).
 * @param installedUpdateId An installed update it JSON string.
 * @returns true on reporting success.
 */
_Bool AzureDeviceUpdateCoreInterface_ReportStateAndResultAsync(
    ADUC_WorkflowDataToken workflowDataToken,
    ADUCITF_State updateState,
    const ADUC_Result* result,
    const char* installedUpdateId);

EXTERN_C_END

#endif // ADUC_ADU_CORE_INTERFACE_H
