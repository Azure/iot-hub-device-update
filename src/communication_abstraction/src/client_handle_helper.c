/**
 * @file client_handle_helper.c 
 * @brief Implements an abstract interface for communicating through the ModuleClient or DeviceClient libaries.
 *  
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */

#include "aduc/client_handle_helper.h"

#include <azureiot/iothub_device_client_ll.h>
#include <azureiot/iothub_module_client_ll.h>

static ADUC_ConnType g_ClientHandleType = ADUC_ConnType_NotSet;

/**
 * Note: Keeping incase there is a need for some static function nonsense
 */
_Bool ClientHandle_SetClientHandleType(ADUC_ConnType type)
{
    if (type != ADUC_ConnType_NotSet && g_ClientHandleType != ADUC_ConnType_NotSet)
    {
        return false;
    }

    g_ClientHandleType = type;

    return true;
}

IOTHUB_DEVICE_CLIENT_LL_HANDLE GetDeviceClientHandle(ADUC_ClientHandle handle)
{
    return *(IOTHUB_DEVICE_CLIENT_LL_HANDLE*)handle;
}

IOTHUB_MODULE_CLIENT_LL_HANDLE GetModuleClientHandle(ADUC_ClientHandle handle)
{
    return *(IOTHUB_MODULE_CLIENT_LL_HANDLE*)handle;
}

/**
 * 
 */
_Bool ClientHandle_CreateFromConnectionString(
    ADUC_ClientHandle* iotHubClientHandle,
    ADUC_ConnType type,
    const char* connectionString,
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    _Bool success = false;

    if (g_ClientHandleType != ADUC_ConnType_NotSet)
    {
        return success;
    }

    g_ClientHandleType = type;

    if (g_ClientHandleType == ADUC_ConnType_Device)
    {
        IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceHandle =
            IoTHubDeviceClient_LL_CreateFromConnectionString(connectionString, protocol);

        *iotHubClientHandle = (ADUC_ClientHandle)malloc(sizeof(IOTHUB_DEVICE_CLIENT_LL_HANDLE));

        memcpy(*iotHubClientHandle, &deviceHandle, sizeof(IOTHUB_DEVICE_CLIENT_LL_HANDLE));

        success = true;
        goto done;
    }
    else if (g_ClientHandleType == ADUC_ConnType_Module)
    {
        IOTHUB_MODULE_CLIENT_LL_HANDLE moduleHandle =
            IoTHubModuleClient_LL_CreateFromConnectionString(connectionString, protocol);

        *iotHubClientHandle = (ADUC_ClientHandle*)malloc(sizeof(IOTHUB_MODULE_CLIENT_LL_HANDLE*));

        memcpy(*iotHubClientHandle, &moduleHandle, sizeof(IOTHUB_MODULE_CLIENT_LL_HANDLE));

        success = true;
        goto done;
    }

done:

    if (!success)
    {
        free(*iotHubClientHandle);
    }

    return success;
}

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

    return result;
}

/**
 * 
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

    return result;
}

/**
 * 
 */
void ClientHandle_DoWork(ADUC_ClientHandle iotHubClientHandle)
{
    if (g_ClientHandleType == ADUC_ConnType_Device)
    {
        IoTHubDeviceClient_LL_DoWork(GetDeviceClientHandle(iotHubClientHandle));
    }
    else if (g_ClientHandleType == ADUC_ConnType_Module)
    {
        IoTHubModuleClient_LL_DoWork(GetModuleClientHandle(iotHubClientHandle));
    }
}

/**
 * 
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

    return result;
}

/**
 * 
 */
IOTHUB_CLIENT_RESULT ClientHandle_SetClientTwinCallback(
    ADUC_ClientHandle iotHubClientHandle,
    IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK moduleTwinCallback,
    void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_INVALID_ARG;

    if (g_ClientHandleType == ADUC_ConnType_Device)
    {
        result = IoTHubDeviceClient_LL_SetDeviceTwinCallback(
            GetDeviceClientHandle(iotHubClientHandle), moduleTwinCallback, userContextCallback);
    }
    else if (g_ClientHandleType == ADUC_ConnType_Module)
    {
        result = IoTHubModuleClient_LL_SetModuleTwinCallback(
            GetModuleClientHandle(iotHubClientHandle), moduleTwinCallback, userContextCallback);
    }

    return result;
}

/**
 * 
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

    return result;
}
/**
 * 
 */
IOTHUB_CLIENT_RESULT ClientHandle_SetDeviceMethodCallback(
    ADUC_ClientHandle iotHubClientHandle,
    IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC moduleMethodCallback,
    void* userContextCallback)
{
    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_INVALID_ARG;

    if (g_ClientHandleType == ADUC_ConnType_Device)
    {
        result = IoTHubDeviceClient_LL_SetDeviceMethodCallback(
            GetDeviceClientHandle(iotHubClientHandle), moduleMethodCallback, userContextCallback);
    }
    else if (g_ClientHandleType == ADUC_ConnType_Module)
    {
        result = IoTHubModuleClient_LL_SetModuleMethodCallback(
            GetModuleClientHandle(iotHubClientHandle), moduleMethodCallback, userContextCallback);
    }

    return result;
}
/**
 * 
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
}
