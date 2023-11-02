/**
 * @file iothub-communication-module.c
 * @brief Implementation for the IoTHub Communication module.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adu_core_interface.h"
#include "aduc/adu_types.h"
#include "aduc/client_handle_helper.h"
#include "du_agent_sdk/agent_module_interface.h"
#if !defined(WIN32)
#    include "aduc/command_helper.h"
#endif
#include "aduc/config_utils.h"
#include "aduc/connection_string_utils.h"
#include "aduc/d2c_messaging.h"
#include "aduc/device_info_interface.h"
#include "aduc/extension_manager.h"
#include "aduc/iothub_communication_manager.h"
#include "aduc/logging.h"
#include "pnp_protocol.h"

// --- NOTE: Following block are codes ported from agent/src/main.c

// Name of ADU Agent subcomponent that this device implements.
static const char g_aduPnPComponentName[] = "deviceUpdate";

// Name of DeviceInformation subcomponent that this device implements.
static const char g_deviceInfoPnPComponentName[] = "deviceInformation";

/**
 * @brief Global IoT Hub client handle.
 */
ADUC_ClientHandle g_iotHubClientHandle = NULL;

//
// Components that this agent supports.
//

/**
 * @brief Function signature for PnP Handler create method.
 */
typedef bool (*PnPComponentCreateFunc)(void** componentContext, int argc, char** argv);

/**
 * @brief Called once after connected to IoTHub (device client handler is valid).
 *
 * DigitalTwin handles aren't valid (and as such no calls may be made on them) until this method is called.
 */
typedef void (*PnPComponentConnectedFunc)(void* componentContext);

/**
 * @brief Function signature for PnP component worker method.
 *        Called regularly after the device client is created.
 *
 * This allows an component implementation to do work in a cooperative multitasking environment.
 */
typedef void (*PnPComponentDoWorkFunc)(void* componentContext);

/**
 * @brief Function signature for PnP component uninitialize method.
 */
typedef void (*PnPComponentDestroyFunc)(void** componentContext);

/**
 * @brief Called when a component's property is updated.
 *
 * @param updateState State to report.
 * @param result Result to report (optional, can be NULL).
 */
typedef void (*PnPComponentPropertyUpdateCallback)(
    ADUC_ClientHandle clientHandle,
    const char* propertyName,
    JSON_Value* propertyValue,
    int version,
    ADUC_PnPComponentClient_PropertyUpdate_Context* sourceContext,
    void* userContextCallback);

static ADUC_PnPComponentClient_PropertyUpdate_Context g_iotHubInitiatedPnPPropertyChangeContext = { false, false };

static ADUC_PnPComponentClient_PropertyUpdate_Context g_deviceInitiatedRetryPnPPropertyChangeContext = { true, true };

/**
 * @brief Defines an PnP Component Client that this agent supports.
 */
typedef struct tagPnPComponentEntry
{
    const char* ComponentName;
    ADUC_ClientHandle* clientHandle;
    const PnPComponentCreateFunc Create;
    const PnPComponentConnectedFunc Connected;
    const PnPComponentDoWorkFunc DoWork;
    const PnPComponentDestroyFunc Destroy;
    const PnPComponentPropertyUpdateCallback
        PnPPropertyUpdateCallback; /**< Called when a component's property is updated. (optional) */
    //
    // Following data is dynamic.
    // Must be initialized to NULL in map and remain last entries in this struct.
    //
    void* Context; /**< Opaque data returned from PnPComponentInitFunc(). */
} PnPComponentEntry;

// clang-format off
/**
 * @brief Interfaces to register.
 *
 * DeviceInfo must be registered before AzureDeviceUpdateCore, as the latter depends on the former.
 */
// NOLINTNEXTLINE(cppcoreguidelines-interfaces-global-init)
static PnPComponentEntry componentList[] = {
    // Important: the 'deviceUpdate' component must before first entry here.
    // This entry will be referenced by ADUC_PnPDeviceTwin_RetryUpdateCommand_Callback function below.
    {
        g_aduPnPComponentName,
        &g_iotHubClientHandleForADUComponent,
        AzureDeviceUpdateCoreInterface_Create,
        AzureDeviceUpdateCoreInterface_Connected,
        AzureDeviceUpdateCoreInterface_DoWork,
        AzureDeviceUpdateCoreInterface_Destroy,
        AzureDeviceUpdateCoreInterface_PropertyUpdateCallback
    },
    {
        g_deviceInfoPnPComponentName,
        &g_iotHubClientHandleForDeviceInfoComponent,
        DeviceInfoInterface_Create,
        DeviceInfoInterface_Connected,
        NULL /* DoWork method - not used */,
        DeviceInfoInterface_Destroy,
        NULL /* PropertyUpdateCallback - not used */
    },
};

/**
 * @brief Uninitialize all PnP components' handler.
 */
static void ADUC_PnP_Components_Destroy()
{
    for (unsigned index = 0; index < ARRAY_SIZE(componentList); ++index)
    {
        PnPComponentEntry* entry = componentList + index;

        if (entry->Destroy != NULL)
        {
            entry->Destroy(&(entry->Context));
        }
    }
}

/**
 * @brief Refreshes the client handle associated with each of the components in the componentList
 *
 * @param clientHandle new handle to be set on each of the components
 */
static void ADUC_PnP_Components_HandleRefresh(ADUC_ClientHandle clientHandle)
{
    Log_Info("Refreshing the handle for the PnP channels.");

    const size_t componentCount = ARRAY_SIZE(componentList);

    for (size_t index = 0; index < componentCount; ++index)
    {
        PnPComponentEntry* entry = componentList + index;

        *(entry->clientHandle) = clientHandle;
    }
}

/**
 * @brief Initialize PnP component client that this agent supports.
 *
 * @param clientHandle the ClientHandle for the IotHub connection
 * @param argc Command-line arguments specific to upper-level handlers.
 * @param argv Size of argc.
 * @return bool True on success.
 */
static bool ADUC_PnP_Components_Create(ADUC_ClientHandle clientHandle, int argc, char** argv)
{
    Log_Info("Initializing PnP components.");
    bool succeeded = false;
    const unsigned componentCount = ARRAY_SIZE(componentList);

    for (unsigned index = 0; index < componentCount; ++index)
    {
        PnPComponentEntry* entry = componentList + index;

        if (!entry->Create(&entry->Context, argc, argv))
        {
            Log_Error("Failed to initialize PnP component '%s'.", entry->ComponentName);
            goto done;
        }

        *(entry->clientHandle) = clientHandle;
    }
    succeeded = true;

done:
    if (!succeeded)
    {
        ADUC_PnP_Components_Destroy();
    }

    return succeeded;
}

/**
 * @brief Callback invoked when a PnP property is updated.
 * @param componentName The name of the component that owns the property.
 * @param propertyName The name of the property.
 * @param propertyValue The value of the property.
 * @param version The version of the property.
 * @param userContextCallback The user context that will be passed to the callback.
 * @return void
 **/
static void ADUC_PnP_ComponentClient_PropertyUpdate_Callback(
    const char* componentName,
    const char* propertyName,
    JSON_Value* propertyValue,
    int version,
    void* userContextCallback)
{
    ADUC_PnPComponentClient_PropertyUpdate_Context* sourceContext =
        (ADUC_PnPComponentClient_PropertyUpdate_Context*)userContextCallback;

    Log_Debug("ComponentName:%s, propertyName:%s", componentName, propertyName);

    if (componentName == NULL)
    {
        // We only support named-components.
        goto done;
    }

    bool supported = false;
    for (unsigned index = 0; index < ARRAY_SIZE(componentList); ++index)
    {
        PnPComponentEntry* entry = componentList + index;

        if (strcmp(componentName, entry->ComponentName) == 0)
        {
            supported = true;
            if (entry->PnPPropertyUpdateCallback != NULL)
            {
                entry->PnPPropertyUpdateCallback(
                    *(entry->clientHandle), propertyName, propertyValue, version, sourceContext, entry->Context);
            }
            else
            {
                Log_Info(
                    "Component name (%s) is recognized but PnPPropertyUpdateCallback is not specfied. Ignoring the property '%s' change event.",
                    componentName,
                    propertyName);
            }
        }
    }

    if (!supported)
    {
        Log_Info("Component name (%s) is not supported by this agent. Ignoring...", componentName);
    }

done:
    return;
}

// Note: This is an array of weak references to componentList[i].componentName
// as such the size of componentList must be equal to the size of g_modeledComponents
static const char* g_modeledComponents[ARRAY_SIZE(componentList)];

static const size_t g_numModeledComponents = ARRAY_SIZE(g_modeledComponents);

static bool g_firstDeviceTwinDataProcessed = false;

/*
 * @brief Initialize the modeled components list.
 */
static void InitializeModeledComponents()
{
    const size_t numModeledComponents = ARRAY_SIZE(g_modeledComponents);

    STATIC_ASSERT(ARRAY_SIZE(componentList) == ARRAY_SIZE(g_modeledComponents));

    for (int i = 0; i < numModeledComponents; ++i)
    {
        g_modeledComponents[i] = componentList[i].ComponentName;
    }
}

/*
 * @brief This function is called when the the 'retry-update' command is received.
 * @param updateState The update state.
 * @param payload The twin payload.
 * @param size The size of the payload.
 * @param userContextCallback The user context that will be passed to the callback.
 * @return void
 */
static void ADUC_PnPDeviceTwin_RetryUpdateCommand_Callback(
    DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char* payload, size_t size, void* userContextCallback)
{
    // Invoke PnP_ProcessTwinData to actually process the data.  PnP_ProcessTwinData uses a visitor pattern to parse
    // the JSON and then visit each property, invoking PnP_TempControlComponent_ApplicationPropertyCallback on each element.
    if (PnP_ProcessTwinData(
            updateState,
            payload,
            size,
            g_modeledComponents,
            1, // Only process the first entry, which is 'deviceUpdate' PnP component.
            ADUC_PnP_ComponentClient_PropertyUpdate_Callback,
            userContextCallback)
        == false)
    {
        // If we're unable to parse the JSON for any reason (typically because the JSON is malformed or we ran out of memory)
        // there is no action we can take beyond logging.
        Log_Error("Unable to process twin JSON.  Ignoring any desired property update requests.");
    }
}

/**
 * @brief Callback invoked when a PnP property is updated.
 * @param updateState The update state.
 * @param payload The twin payload.
 * @param size The size of the payload.
 * @param userContextCallback The user context that will be passed to the callback.
 * @return void
 *
 */
static void ADUC_PnPDeviceTwin_Callback(
    DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char* payload, size_t size, void* userContextCallback)
{
    // Invoke PnP_ProcessTwinData to actually process the data.  PnP_ProcessTwinData uses a visitor pattern to parse
    // the JSON and then visit each property, invoking PnP_TempControlComponent_ApplicationPropertyCallback on each element.
    if (PnP_ProcessTwinData(
            updateState,
            payload,
            size,
            g_modeledComponents,
            g_numModeledComponents,
            ADUC_PnP_ComponentClient_PropertyUpdate_Callback,
            userContextCallback)
        == false)
    {
        // If we're unable to parse the JSON for any reason (typically because the JSON is malformed or we ran out of memory)
        // there is no action we can take beyond logging.
        Log_Error("Unable to process twin JSON.  Ignoring any desired property update requests.");
    }

    if (!g_firstDeviceTwinDataProcessed)
    {
        g_firstDeviceTwinDataProcessed = true;

        Log_Info("Processing existing Device Twin data after agent started.");

        const unsigned componentCount = ARRAY_SIZE(componentList);
        Log_Debug("Notifies components that all callback are subscribed.");
        for (unsigned index = 0; index < componentCount; ++index)
        {
            PnPComponentEntry* entry = componentList + index;
            if (entry->Connected != NULL)
            {
                entry->Connected(entry->Context);
            }
        }
    }
}

#ifdef ADUC_COMMAND_HELPER_H

/**
 * @brief Invokes PnPHandleCommandCallback on every PnPComponentEntry.
 *
 * @param command The string contains command (and options) from other component or process.
 * @param commandContext A data context associated with the command.
 * @return bool
 */
static bool RetryUpdateCommandHandler(const char* command, void* commandContext)
{
    UNREFERENCED_PARAMETER(command);
    UNREFERENCED_PARAMETER(commandContext);
    IOTHUB_CLIENT_RESULT iothubResult = ClientHandle_GetTwinAsync(
        g_iotHubClientHandle,
        ADUC_PnPDeviceTwin_RetryUpdateCommand_Callback,
        &g_deviceInitiatedRetryPnPPropertyChangeContext);

    return iothubResult == IOTHUB_CLIENT_OK;
}

// This command can be use by other process, to tell a DU agent to retry the current update, if exist.
ADUC_Command redoUpdateCommand = { "retry-update", RetryUpdateCommandHandler };

#endif // #ifdef ADUC_COMMAND_HELPER_H

ADUC_AGENT_CONTRACT_INFO g_iotHubClientContractInfo = {
    "Microsoft",
    "IoT Hub Client Module",
    1,
    "Microsoft/IotHubClientModule:1"
};

/**
 * @brief Gets the extension contract info.
 * @param handle The handle to the module. This is the same handle that was returned by the Create function.
 * @return ADUC_AGENT_CONTRACT_INFO The extension contract info.
 */
const ADUC_AGENT_CONTRACT_INFO* IoTHubClientModule_GetContractInfo(ADUC_AGENT_MODULE_HANDLE handle)
{
    IGNORED_PARAMETER(handle);
    return &g_iotHubClientContractInfo;
}

/**
 * @brief Initialize the IoTHub Client module.
*/
int IoTHubClientModule_Initialize(ADUC_AGENT_MODULE_HANDLE handle, void* moduleInitData)
{
    IGNORED_PARAMETER(handle);
    IGNORED_PARAMETER(moduleInitData);

    ADUC_Result result = { ADUC_GeneralResult_Failure };

    ADUC_ConnectionInfo info = {};

    // Need to set ret and goto done after this to ensure proper shutdown and deinitialization.
    // TODO (nox-msft) - set log level according to global config.
    ADUC_Logging_Init(0, "iothub-client-module");

    InitializeModeledComponents();

    if (!IoTHub_CommunicationManager_Init(
                &g_iotHubClientHandle,
                ADUC_PnPDeviceTwin_Callback,
                ADUC_PnP_Components_HandleRefresh,
                &g_iotHubInitiatedPnPPropertyChangeContext))
    {
        Log_Error("IoTHub_CommunicationManager_Init failed");
        goto done;
    }

    if (!ADUC_D2C_Messaging_Init())
    {
        goto done;
    }

    if (!GetAgentConfigInfo(&info))
    {
        goto done;
    }

    if (!IoTHub_CommunicationManager_Init(
            &g_iotHubClientHandle,
            ADUC_PnPDeviceTwin_Callback,
            ADUC_PnP_Components_HandleRefresh,
            &g_iotHubInitiatedPnPPropertyChangeContext))
    {
        Log_Error("IoTHub_CommunicationManager_Init failed");
        goto done;
    }

    // TODO: nox-msft - Instead of passing launchArgs, we should configure agent behaviors (e.g. IoTHub logging)
    // via a configuration file. This will allow us to remove the launchArgs parameter from this function.
    if (!ADUC_PnP_Components_Create(g_iotHubClientHandle, 0, NULL))
    {
        Log_Error("ADUC_PnP_Components_Create failed");
        goto done;
    }

    // The connection string is valid (IoT hub connection successful) and we are ready for further processing.
    // Send connection string to DO SDK for it to discover the Edge gateway if present.
    if (ConnectionStringUtils_IsNestedEdge(info.connectionString))
    {
        result = ExtensionManager_InitializeContentDownloader(info.connectionString);
    }
    else
    {
        result = ExtensionManager_InitializeContentDownloader(NULL /*initializeData*/);
    }

#ifdef ADUC_COMMAND_HELPER_H
    if (InitializeCommandListenerThread())
    {
        RegisterCommand(&redoUpdateCommand);
    }
    else
    {
        Log_Error(
            "Cannot initialize the command listener thread. Running another instance of DU Agent with --command will not work correctly.");
        // Note: even though we can't create command listener here, we need to ensure that
        // the agent stay alive and connected to the IoT hub.
    }
#endif // #ifdef ADUC_COMMAND_HELPER_H

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        // Since it is nested edge and if DO fails to accept the connection string, then we go ahead and
        // fail the startup.
        Log_Error("Failed to set DO connection string in Nested Edge scenario, result: 0x%08x", result.ResultCode);
        goto done;
    }

    result.ResultCode = ADUC_GeneralResult_Success;
    result.ExtendedResultCode = 0;

done:
    ADUC_ConnectionInfo_DeAlloc(&info);
    return result.ResultCode == ADUC_GeneralResult_Success ? 0 : -1;
}

/**
 * @brief Deinitialize the IoTHub Client module.
 *
 * @param module The handle to the module. This is the same handle that was returned by the Create function.
 *
 * @return int 0 on success.
 */
int IoTHubClientModule_Deinitialize(ADUC_AGENT_MODULE_HANDLE module)
{
    Log_Info("Deinitialize");
    ADUC_D2C_Messaging_Uninit();
#ifdef ADUC_COMMAND_HELPER_H
    UninitializeCommandListenerThread();
#endif
    ADUC_PnP_Components_Destroy();
    IoTHub_CommunicationManager_Deinit();
    ADUC_Logging_Uninit();
    // TODO: nox-msft Unload all extension
    return 0;
}

/**
 * @brief Create a Device Update Agent Module for IoT Hub PnP Client.
 *
 * @return ADUC_AGENT_MODULE_HANDLE The handle to the module.
 */
ADUC_AGENT_MODULE_HANDLE IoTHubClientModule_Create()
{
    ADUC_AGENT_MODULE_HANDLE handle = &g_iotHubClientHandle;
    return handle;
}

/*
 * @brief Destroy the Device Update Agent Module for IoT Hub PnP Client.
 * @param handle The handle to the module. This is the same handle that was returned by the Create function.
 */
void IoTHubClientModule_Destroy(ADUC_AGENT_MODULE_HANDLE handle)
{
    IGNORED_PARAMETER(handle);
}

/**
 * @brief Perform the work for the extension. This must be a non-blocking operation.
 *
 * @return ADUC_Result
 */
int IoTHubClientModule_DoWork(ADUC_AGENT_MODULE_HANDLE handle)
{
    if (handle != &g_iotHubClientHandle)
    {
        goto done;
    }

    // If any components have requested a DoWork callback, regularly call it.
    for (unsigned index = 0; index < ARRAY_SIZE(componentList); ++index)
    {
        PnPComponentEntry* entry = componentList + index;

        if (entry->DoWork != NULL)
        {
            entry->DoWork(entry->Context);
        }
    }

    ADUC_D2C_Messaging_DoWork();

    // NOTE: When using low level samples (iothub_ll_*), the IoTHubDeviceClient_LL_DoWork
    // function must be called regularly (eg. every 100 milliseconds) for the IoT device client to work properly.
    // See: https://github.com/Azure/azure-iot-sdk-c/tree/master/iothub_client/samples
    // NOTE: For this example the above has been wrapped to support module and device client methods using
    // the client_handle_helper.h function ClientHandle_DoWork()
    IoTHub_CommunicationManager_DoWork(&g_iotHubClientHandle);

done:
    return 0;
}

ADUC_AGENT_MODULE_INTERFACE IoTHubClientModuleInterface = {
    &g_iotHubClientHandle,
    IoTHubClientModule_Destroy,
    IoTHubClientModule_GetContractInfo,
    IoTHubClientModule_DoWork,
    IoTHubClientModule_Initialize,
    IoTHubClientModule_Deinitialize,
};
