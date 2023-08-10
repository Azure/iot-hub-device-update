/**
 * @file iothub-communication-module.c
 * @brief Implementation for the IoTHub Communication module.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/agent-workflow-module.h"
#include "aduc/adu_core_interface.h"
#include "aduc/client_handle_helper.h"
#include "aduc/command_helper.h"
#include "aduc/config_utils.h"
#include "aduc/connection_string_utils.h"
#include "aduc/device_info_interface.h"
#include "aduc/d2c_messaging.h"
#include "aduc/extension_manager.h"
#include "aduc/iothub_communication_manager.h"
#include "aduc/logging.h"
#include "pnp_protocol.h"

// TODO (nox-msft) - enable diagnostic features
// #include <diagnostics_devicename.h>
// #include <diagnostics_interface.h>


// --- NOTE: Following block are codes ported from agent/src/main.c

// Name of ADU Agent subcomponent that this device implements.
static const char g_aduPnPComponentName[] = "deviceUpdate";

// Name of DeviceInformation subcomponent that this device implements.
static const char g_deviceInfoPnPComponentName[] = "deviceInformation";

// TODO (nox-msft) - enable diagnostic features
//// Name of the Diagnostics subcomponent that this device is using
// static const char g_diagnosticsPnPComponentName[] = "diagnosticInformation";

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

    // TODO (nox-msft) - enable diagnostic features
    // {
    //     g_diagnosticsPnPComponentName,
    //     &g_iotHubClientHandleForDiagnosticsComponent,
    //     DiagnosticsInterface_Create,
    //     DiagnosticsInterface_Connected,
    //     NULL /* DoWork method - not used */,
    //     DiagnosticsInterface_Destroy,
    //     DiagnosticsInterface_PropertyUpdateCallback
    // },
};


// TODO (nox-mfst) - enable diagnostic features
// /**
//  * @brief Sets the Diagnostic DeviceName for creating the device's diagnostic container
//  * @param connectionString connectionString to extract the device-id and module-id from
//  * @returns true on success; false on failure
//  */
// bool ADUC_SetDiagnosticsDeviceNameFromConnectionString(const char* connectionString)
// {
//     bool succeeded = false;

//     char* deviceId = NULL;

//     char* moduleId = NULL;

//     if (!ConnectionStringUtils_GetDeviceIdFromConnectionString(connectionString, &deviceId))
//     {
//         goto done;
//     }

//     // Note: not all connection strings have a module-id
//     ConnectionStringUtils_GetModuleIdFromConnectionString(connectionString, &moduleId);

//     if (!DiagnosticsComponent_SetDeviceName(deviceId, moduleId))
//     {
//         goto done;
//     }

//     succeeded = true;

// done:

//     free(deviceId);
//     free(moduleId);
//     return succeeded;
// }

//
// IotHub methods.
//

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

//
// ADUC_PnP_ComponentClient_PropertyUpdate_Callback is the callback function that the PnP helper layer invokes per property update.
//
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

static void InitializeModeledComponents()
{
    const size_t numModeledComponents = ARRAY_SIZE(g_modeledComponents);

    STATIC_ASSERT(ARRAY_SIZE(componentList) == ARRAY_SIZE(g_modeledComponents));

    for (int i = 0; i < numModeledComponents; ++i)
    {
        g_modeledComponents[i] = componentList[i].ComponentName;
    }
}

//
// ADUC_PnP_DeviceTwin_Callback is invoked by IoT SDK when a twin - either full twin or a PATCH update - arrives.
//
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

//
// ADUC_PnP_DeviceTwin_Callback is invoked by IoT SDK when a twin - either full twin or a PATCH update - arrives.
//
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


// --- End (agent/src/main.c ported codes)

/**
 * @brief Gets the extension contract info.
 *
 * @param[out] contractInfo The extension contract info.
 * @return ADUC_Result The result.
 */
ADUC_Result GetContractInfo(char** contractInfo)
{
    IGNORED_PARAMETER(contractInfo);
    ADUC_Result result = {0};
    return result;
}

ADUC_DeviceUpdateAgentModule g_iotHubAgentModule;

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

#if !ADU_GEN2
    IOTHUB_CLIENT_RESULT iothubResult = ClientHandle_GetTwinAsync(
        g_iotHubClientHandle,
        ADUC_PnPDeviceTwin_RetryUpdateCommand_Callback,
        &g_deviceInitiatedRetryPnPPropertyChangeContext);
    return iothubResult == IOTHUB_CLIENT_OK;
#else
#endif

}

// This command can be use by other process, to tell a DU agent to retry the current update, if exist.
ADUC_Command redoUpdateCommand = { "retry-update", RetryUpdateCommandHandler };



/**
 * @brief Create a Device Update Agent Module object
 *
 * @param deviceUpdateAgentModule A pointer to the Device Update Agent Module object.
 * @return ADUC_Result
 */
ADUC_Result CreateDeviceUpdateAgentModule(ADUC_DeviceUpdateAgentModule** deviceUpdateAgentModule)
{
    ADUC_Result result = {ADUC_GeneralResult_Failure};
    IGNORED_PARAMETER(deviceUpdateAgentModule);
    if (deviceUpdateAgentModule == NULL)
    {
        goto done;
    }

    *deviceUpdateAgentModule = &g_iotHubAgentModule;

    result.ResultCode = ADUC_GeneralResult_Success;
done:
    return result;
}

/**
 * @brief Perform the work for the extension. This must be a non-blocking operation.
 *
 * @return ADUC_Result
 */
ADUC_Result DoWork(ADUC_DeviceUpdateAgentModule* module)
{
    ADUC_Result result = {ADUC_GeneralResult_Failure};

    if (module != &g_iotHubAgentModule)
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
    return result;
}

/**
 * @brief Initialize the module. This is called once when the module is loaded.
 *
 * @param module The module to initialize.
 * @param moduleInitData The module initialization data. Consult the documentation for the specific module for details.
 * @return ADUC_Result
 */
ADUC_Result InitializeModule(ADUC_DeviceUpdateAgentModule* module, void *moduleInitData)
{
     IGNORED_PARAMETER(module);
     IGNORED_PARAMETER(moduleInitData);

    ADUC_Result result = { ADUC_GeneralResult_Failure };

    ADUC_ConnectionInfo info = {};

    // Need to set ret and goto done after this to ensure proper shutdown and deinitialization.
    // TODO (nox-msft) - set log level according to global config.
    ADUC_Logging_Init(0, "iothub-comm-module");

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

#if ADUC_SUPPORT_CONNECTION_STRING_CMD_ARG
    // TODO (nox-msft) - support '-c' option.
    if (launchArgs->connectionString != NULL)
    {
        ADUC_ConnType connType = GetConnTypeFromConnectionString(launchArgs->connectionString);

        if (connType == ADUC_ConnType_NotSet)
        {
            Log_Error("Connection string is invalid");
            goto done;
        }

        ADUC_ConnectionInfo connInfo = {
            ADUC_AuthType_NotSet, connType, launchArgs->connectionString, NULL, NULL, NULL
        };

        if (!ADUC_SetDiagnosticsDeviceNameFromConnectionString(connInfo.connectionString))
        {
            Log_Error("Setting DiagnosticsDeviceName failed");
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
    }
    else
#endif

    {
        if (!GetAgentConfigInfo(&info))
        {
            goto done;
        }

        // TODO (nox-msft) - enable diagnostics feature
        // if (!ADUC_SetDiagnosticsDeviceNameFromConnectionString(info.connectionString))
        // {
        //     Log_Error("Setting DiagnosticsDeviceName failed");
        //     goto done;
        // }

        if (!IoTHub_CommunicationManager_Init(
                &g_iotHubClientHandle,
                ADUC_PnPDeviceTwin_Callback,
                ADUC_PnP_Components_HandleRefresh,
                &g_iotHubInitiatedPnPPropertyChangeContext))
        {
            Log_Error("IoTHub_CommunicationManager_Init failed");
            goto done;
        }
    }

    // TODO (nox-msft) - pass command line arg
    // if (!ADUC_PnP_Components_Create(g_iotHubClientHandle, launchArgs->argc, launchArgs->argv))
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
    return result;
}

/**
 * @brief
 *
 * @param module
 *
 * @return ADUC_Result
 */
ADUC_Result DeinitializeModule(ADUC_DeviceUpdateAgentModule* module)
{
     IGNORED_PARAMETER(module);
    ADUC_Result result = {0};

    UninitializeCommandListenerThread();
    ADUC_PnP_Components_Destroy();
    ADUC_D2C_Messaging_Uninit();
    IoTHub_CommunicationManager_Deinit();
    // TODO (nox-msft) - enable diagnostics feature
    // DiagnosticsComponent_DestroyDeviceName();
    return result;
}


/**
 * @brief Get the Data object for the specified key.
 *
 * @param module  name of the module
 * @param dataType data type
 * @param key data key/name
 * @param value return value (call must free the memory of the return value once done with it)
 * @param size return size of the return value
 * @return ADUC_Result
 */
ADUC_Result GetData(ADUC_DeviceUpdateAgentModule* module, ADUC_ModuleDataType dataType, const char* key, void** value, int* size)
{
    IGNORED_PARAMETER(module);
    IGNORED_PARAMETER(dataType);
    IGNORED_PARAMETER(key);
    IGNORED_PARAMETER(value);
    IGNORED_PARAMETER(size);
    ADUC_Result result = {0};
    return result;
}
