/**
 * @file client_handle_helper.c
 * @brief Implements an abstract interface for communicating through the ModuleClient or DeviceClient libraries.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/client_handle_helper.h"

#include <aduc/logging.h>
#include <azureiot/iothub_device_client_ll.h>
#include <azureiot/iothub_module_client_ll.h>

static ADUC_ConnType g_ClientHandleType = ADUC_ConnType_NotSet;

/**
 * @brief Safely casts @p handle to an IOTHUB_DEVICE_CLIENT_LL_HANDLE
 * @param handle the pointer to be cast
 * @returns NULL if @p handle is NULL otherwise returns the cast pointer
 */
IOTHUB_DEVICE_CLIENT_LL_HANDLE GetDeviceClientHandle(ADUC_ClientHandle handle)
{
    if (handle == NULL || g_ClientHandleType != ADUC_ConnType_Device)
    {
        return NULL;
    }

    return (IOTHUB_DEVICE_CLIENT_LL_HANDLE)handle;
}

/**
 * @brief Safely casts @p handle to an IOTHUB_MODULE_CLIENT_LL_HANDLE
 * @param handle the pointer to be cast
 * @returns NULL if @p handle is NULL otherwise returns the cast pointer
 */
IOTHUB_MODULE_CLIENT_LL_HANDLE GetModuleClientHandle(ADUC_ClientHandle handle)
{
    if (handle == NULL || g_ClientHandleType != ADUC_ConnType_Module)
    {
        return NULL;
    }

    return (IOTHUB_MODULE_CLIENT_LL_HANDLE)handle;
}

/**
 * @brief Wrapper function for the Device and Module CreateFromConnectionString functions
 * @details Uses either the device or module function depending on what the client type has been set to.
 * @param iotHubClientHandle the clientHandle to be set by the createFromConnectionString function
 * @param connectionString connectionString that will be used to create the client connection
 * @param protocol the protocol to use to create the client connection
 * @returns true on success false on failure
 */
bool ClientHandle_CreateFromConnectionString(
    ADUC_ClientHandle* iotHubClientHandle,
    ADUC_ConnType type,
    const char* connectionString,
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    if (g_ClientHandleType != ADUC_ConnType_NotSet)
    {
        Log_Error(
            "ClientHandle_CreateFromConnectionString called for a second time. Only supports a single connection per agent");
        return false;
    }

    if (connectionString == NULL || iotHubClientHandle == NULL)
    {
        Log_Error("ClientHandle_CreateFromConnectionString called with NULL parameters");
        return false;
    }

    if (type == ADUC_ConnType_Device)
    {
        IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceHandle =
            IoTHubDeviceClient_LL_CreateFromConnectionString(connectionString, protocol);

        *iotHubClientHandle = (ADUC_ClientHandle)deviceHandle;
        goto done;
    }
    else if (type == ADUC_ConnType_Module)
    {
        IOTHUB_MODULE_CLIENT_LL_HANDLE moduleHandle =
            IoTHubModuleClient_LL_CreateFromConnectionString(connectionString, protocol);

        *iotHubClientHandle = (ADUC_ClientHandle)moduleHandle;
        goto done;
    }
    else
    {
        *iotHubClientHandle = NULL;
        Log_Error("Invalid call of ClientHandle_CreateFromConnectionString without a valid ADUC_ConnType");
        goto done;
    }

done:

    if (*iotHubClientHandle == NULL)
    {
        Log_Error("Call to CreateFromConnectionString returned NULL");
        return false;
    }

    g_ClientHandleType = type;
    return true;
}

/**
 * @brief Wrapper function for the Device and Module SetConnectionStatusCallback function
 * @details Uses either the device or module function depending on what the client type has been set to.
 * @param iotHubClientHandle the clientHandle to be used for the operation
 * @param connectionStatusCallback the callback for the connection status
 * @param userContextCallback context for the callback usually a tracking data structure
 * @returns a value of IOTHUB_CLIENT_RESULT
 */
IOTHUB_CLIENT_RESULT ClientHandle_SetConnectionStatusCallback(
    ADUC_ClientHandle iotHubClientHandle,
    IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK connectionStatusCallback,
    void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_INVALID_ARG;

    if (g_ClientHandleType == ADUC_ConnType_Device)
    {
        result = IoTHubDeviceClient_LL_SetConnectionStatusCallback(
            GetDeviceClientHandle(iotHubClientHandle), connectionStatusCallback, userContextCallback);
    }
    else if (g_ClientHandleType == ADUC_ConnType_Module)
    {
        result = IoTHubModuleClient_LL_SetConnectionStatusCallback(
            GetModuleClientHandle(iotHubClientHandle), connectionStatusCallback, userContextCallback);
    }
    else
    {
        Log_Error("ClientHandle_SetConnectionStatusCallback before called ClientHandle_CreateFromConnectionString");
    }

    return result;
}

/**
 * @brief Wrapper for the Device and Module SendEventAsync functions
 * @details Uses either the device or module function depending on what the client type has been set to.
 * @param iotHubClientHandle clientHandle to be used for the operation
 * @param eventMessageHandle Message handle to be sent to the IotHub
 * @param eventConfirmationCallback Callback to be used once the message has been sent
 * @param userContextCallback Parameter to be passed to @p eventConfirmationCallback once the message has been sent
 * @returns a value of IOTHUB_CLIENT_RESULT
 */
IOTHUB_CLIENT_RESULT ClientHandle_SendEventAsync(
    ADUC_ClientHandle iotHubClientHandle,
    IOTHUB_MESSAGE_HANDLE eventMessageHandle,
    IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback,
    void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_INVALID_ARG;

    if (g_ClientHandleType == ADUC_ConnType_Device)
    {
        result = IoTHubDeviceClient_LL_SendEventAsync(
            GetDeviceClientHandle(iotHubClientHandle),
            eventMessageHandle,
            eventConfirmationCallback,
            userContextCallback);
    }
    else if (g_ClientHandleType == ADUC_ConnType_Module)
    {
        result = IoTHubModuleClient_LL_SendEventAsync(
            GetModuleClientHandle(iotHubClientHandle),
            eventMessageHandle,
            eventConfirmationCallback,
            userContextCallback);
    }
    else
    {
        Log_Error("ClientHandle_SendEventAsync before called ClientHandle_CreateFromConnectionString");
    }
    return result;
}

/**
 * @brief Wrapper for the Device and Module DoWork functions
 * @details Uses either the device or module function depending on what the client type has been set to.
 * @param iotHubClientHandle the clientHandle to be used for the operation
 */
void ClientHandle_DoWork(ADUC_ClientHandle iotHubClientHandle)
{
    if (iotHubClientHandle == NULL)
    {
        return;
    }

    if (g_ClientHandleType == ADUC_ConnType_Device)
    {
        IoTHubDeviceClient_LL_DoWork(GetDeviceClientHandle(iotHubClientHandle));
    }
    else if (g_ClientHandleType == ADUC_ConnType_Module)
    {
        IoTHubModuleClient_LL_DoWork(GetModuleClientHandle(iotHubClientHandle));
    }
    else
    {
        Log_Error("ClientHandle_DoWork before called ClientHandle_CreateFromConnectionString");
    }
}

/**
 * @brief Wrapper for the Device and Module SetOption functions
 * @details Uses either the device or module function depending on what the client type has been set to.
 * @param iotHubClientHandle ADUC_ClientHandle to be used for the operation
 * @param optionName Name of the option to be set
 * @param value Value of the option to be set
 * @returns a value of IOTHUB_CLIENT_RESULT
 */
IOTHUB_CLIENT_RESULT
ClientHandle_SetOption(ADUC_ClientHandle iotHubClientHandle, const char* optionName, const void* value)
{
    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_INVALID_ARG;

    if (g_ClientHandleType == ADUC_ConnType_Device)
    {
        result = IoTHubDeviceClient_LL_SetOption(GetDeviceClientHandle(iotHubClientHandle), optionName, value);
    }
    else if (g_ClientHandleType == ADUC_ConnType_Module)
    {
        result = IoTHubModuleClient_LL_SetOption(GetModuleClientHandle(iotHubClientHandle), optionName, value);
    }
    else
    {
        Log_Error("ClientHandle_SetOption before called ClientHandle_CreateFromConnectionString");
    }

    return result;
}

/**
 * @brief Wrapper for the device or model GetTwinAsync functions
 * @details Uses either the device or module function depending on what the client type has been set to.
 * @param iotHubClientHandle The clientHandle to be used for the operation
 * @param deviceTwinCallback Callback for when the function completes
 * @param userContextCallback A parameter to @p deviceTwinCallback
 * @returns a value of IOTHUB_CLIENT_RESULT
 */
IOTHUB_CLIENT_RESULT
ClientHandle_GetTwinAsync(
    ADUC_ClientHandle iotHubClientHandle,
    IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback,
    void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_INVALID_ARG;
    if (g_ClientHandleType == ADUC_ConnType_Device)
    {
        result = IoTHubDeviceClient_LL_GetTwinAsync(
            GetDeviceClientHandle(iotHubClientHandle), deviceTwinCallback, userContextCallback);
    }
    else if (g_ClientHandleType == ADUC_ConnType_Module)
    {
        result = IoTHubModuleClient_LL_GetTwinAsync(
            GetModuleClientHandle(iotHubClientHandle), deviceTwinCallback, userContextCallback);
    }
    else
    {
        Log_Error("ClientHandle_SetOption before called ClientHandle_CreateFromConnectionString");
    }

    return result;
}

/**
 * @brief Wrapper for the Device and Module SetClientTwinCallback functions
 * @details Uses either the device or module function depending on what the client type has been set to.
 * @param iotHubClientHandle The clientHandle to be used for the operation
 * @param deviceTwinCallback Callback for when the function completes
 * @param userContextCallback A parameter to @p deviceTwinCallback
 * @returns a value of IOTHUB_CLIENT_RESULT
 */
IOTHUB_CLIENT_RESULT ClientHandle_SetClientTwinCallback(
    ADUC_ClientHandle iotHubClientHandle,
    IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK deviceTwinCallback,
    void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_INVALID_ARG;

    if (g_ClientHandleType == ADUC_ConnType_Device)
    {
        result = IoTHubDeviceClient_LL_SetDeviceTwinCallback(
            GetDeviceClientHandle(iotHubClientHandle), deviceTwinCallback, userContextCallback);
    }
    else if (g_ClientHandleType == ADUC_ConnType_Module)
    {
        result = IoTHubModuleClient_LL_SetModuleTwinCallback(
            GetModuleClientHandle(iotHubClientHandle), deviceTwinCallback, userContextCallback);
    }
    else
    {
        Log_Error("ClientHandle_SetClientTwinCallback before called ClientHandle_CreateFromConnectionString");
    }

    return result;
}

/**
 * @brief Wrapper for the Device and Module SendReportedState functions
 * @details Uses either the device or module function depending on what the client type has been set to.
 * @param iotHubClientHandle the clientHandle to be used for the operation
 * @param reportedState reportedState to send to the IotHub
 * @param size the size of @p reportedState
 * @param reportedStateCallback callback for when the call is completed
 * @param userContextCallback the parameter that will be passed to @p reportedStateCallback
 * @returns a value of IOTHUB_CLIENT_RESULT
 */
IOTHUB_CLIENT_RESULT ClientHandle_SendReportedState(
    ADUC_ClientHandle iotHubClientHandle,
    const unsigned char* reportedState,
    size_t size,
    IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reportedStateCallback,
    void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_INVALID_ARG;

    if (g_ClientHandleType == ADUC_ConnType_Device)
    {
        result = IoTHubDeviceClient_LL_SendReportedState(
            GetDeviceClientHandle(iotHubClientHandle),
            reportedState,
            size,
            reportedStateCallback,
            userContextCallback);
    }
    else if (g_ClientHandleType == ADUC_ConnType_Module)
    {
        result = IoTHubModuleClient_LL_SendReportedState(
            GetModuleClientHandle(iotHubClientHandle),
            reportedState,
            size,
            reportedStateCallback,
            userContextCallback);
    }
    else
    {
        Log_Error("ClientHandle_SendReportedState before called ClientHandle_CreateFromConnectionString");
    }

    return result;
}

/**
 * @brief Wrapper for the Device and Module SetDeviceMethodCallback functions
 * @details Uses either the device or module function depending on what the client type has been set to.
 * @param iotHubClientHandle the clientHandle to be used for the operation
 * @param deviceMethodCallback callback for  when the device method is set
 * @param userContextCallback the argument that will be passed to @p deviceMethodCallback
 * @returns a value of IOTHUB_CLIENT_RESULT
 */
IOTHUB_CLIENT_RESULT ClientHandle_SetDeviceMethodCallback(
    ADUC_ClientHandle iotHubClientHandle,
    IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC deviceMethodCallback,
    void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_INVALID_ARG;

    if (g_ClientHandleType == ADUC_ConnType_Device)
    {
        result = IoTHubDeviceClient_LL_SetDeviceMethodCallback(
            GetDeviceClientHandle(iotHubClientHandle), deviceMethodCallback, userContextCallback);
    }
    else if (g_ClientHandleType == ADUC_ConnType_Module)
    {
        result = IoTHubModuleClient_LL_SetModuleMethodCallback(
            GetModuleClientHandle(iotHubClientHandle), deviceMethodCallback, userContextCallback);
    }
    else
    {
        Log_Error("ClientHandle_SetDeviceMethodCallback before called ClientHandle_CreateFromConnectionString");
    }

    return result;
}

/**
 * @brief Wrapper for the Device and Module Destroy functions
 * @details Uses either the device or module function depending on what the client type has been set to.
 * @param iotHubClientHandle the clientHandle to be used for the operation
 */
void ClientHandle_Destroy(ADUC_ClientHandle iotHubClientHandle)
{
    if (g_ClientHandleType == ADUC_ConnType_Device)
    {
        IoTHubDeviceClient_LL_Destroy(GetDeviceClientHandle(iotHubClientHandle));
    }
    else if (g_ClientHandleType == ADUC_ConnType_Module)
    {
        IoTHubModuleClient_LL_Destroy(GetModuleClientHandle(iotHubClientHandle));
    }
    else
    {
        Log_Error("ClientHandle_Destroy before called ClientHandle_CreateFromConnectionString");
    }

    g_ClientHandleType = ADUC_ConnType_NotSet;
}
