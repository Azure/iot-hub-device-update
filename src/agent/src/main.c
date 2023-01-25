/**
 * @file main.c
 * @brief Implements the main code for the Device Update Agent.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/adu_core_export_helpers.h"
#include "aduc/adu_core_interface.h"
#include "aduc/adu_types.h"
#include "aduc/agent_workflow.h"
#include "aduc/c_utils.h"
#include "aduc/client_handle_helper.h"
#include "aduc/command_helper.h"
#include "aduc/config_utils.h"
#include "aduc/connection_string_utils.h"
#include "aduc/d2c_messaging.h"
#include "aduc/device_info_interface.h"
#include "aduc/extension_manager.h"
#include "aduc/extension_utils.h"
#include "aduc/health_management.h"
#include "aduc/https_proxy_utils.h"
#include "aduc/iothub_communication_manager.h"
#include "aduc/logging.h"
#include "aduc/permission_utils.h"
#include "aduc/string_c_utils.h"
#include "aduc/system_utils.h"
#include <azure_c_shared_utility/shared_util_options.h>
#include <azure_c_shared_utility/threadapi.h> // ThreadAPI_Sleep
#include <ctype.h>
#include <do_config.h>
#include <diagnostics_devicename.h>
#include <diagnostics_interface.h>
#include <getopt.h>
#include <iothub_client_options.h>
#include <pnp_protocol.h>

#ifdef ADUC_ALLOW_MQTT
#    include <iothubtransportmqtt.h>
#endif

#ifdef ADUC_ALLOW_MQTT_OVER_WEBSOCKETS
#    include <iothubtransportmqtt_websockets.h>
#endif

#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h> // strtol
#include <sys/stat.h>
#include <unistd.h>

#include "pnp_protocol.h"

#include "eis_utils.h"

/**
 * @brief Make getopt* stop parsing as soon as non-option argument is encountered.
 * @remark See GETOPT.3 man page for more details.
 */
#define STOP_PARSE_ON_NONOPTION_ARG "+"

/**
 * @brief Make getopt* return a colon instead of question mark when an option is missing its corresponding option argument, so we can distinguish these scenarios.
 * Also, this suppresses printing of an error message.
 * @remark It must come directly after a '+' or '-' first character in the optionstring.
 * See GETOPT.3 man page for more details.
 */
#define RET_COLON_FOR_MISSING_OPTIONARG ":"

// Name of ADU Agent subcomponent that this device implements.
static const char g_aduPnPComponentName[] = "deviceUpdate";

// Name of DeviceInformation subcomponent that this device implements.
static const char g_deviceInfoPnPComponentName[] = "deviceInformation";

// Name of the Diagnostics subcomponent that this device is using
static const char g_diagnosticsPnPComponentName[] = "diagnosticInformation";

ADUC_LaunchArguments launchArgs;

/**
 * @brief Global IoT Hub client handle.
 */
ADUC_ClientHandle g_iotHubClientHandle = NULL;

/**
 * @brief Determines if we're shutting down.
 *
 * Value indicates the shutdown signal if shutdown requested.
 *
 * Do not shutdown = 0; SIGINT = 2; SIGTERM = 15;
 */
static int g_shutdownSignal = 0;

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
    {
        g_diagnosticsPnPComponentName,
        &g_iotHubClientHandleForDiagnosticsComponent,
        DiagnosticsInterface_Create,
        DiagnosticsInterface_Connected,
        NULL /* DoWork method - not used */,
        DiagnosticsInterface_Destroy,
        DiagnosticsInterface_PropertyUpdateCallback
    },
};

// clang-format on
ADUC_ExtensionRegistrationType GetRegistrationTypeFromArg(const char* arg)
{
    if (strcmp(arg, "updateContentHandler") == 0)
    {
        return ExtensionRegistrationType_UpdateContentHandler;
    }

    if (strcmp(arg, "contentDownloader") == 0)
    {
        return ExtensionRegistrationType_ContentDownloadHandler;
    }

    if (strcmp(arg, "componentEnumerator") == 0)
    {
        return ExtensionRegistrationType_ComponentEnumerator;
    }

    if (strcmp(arg, "downloadHandler") == 0)
    {
        return ExtensionRegistrationType_DownloadHandler;
    }

    return ExtensionRegistrationType_None;
}

/**
 * @brief Parse command-line arguments.
 * @param argc arguments count.
 * @param argv arguments array.
 * @param launchArgs a struct to store the parsed arguments.
 *
 * @return 0 if succeeded without additional non-option args,
 * -1 on failure,
 *  or positive index to argv index where additional args start on success with additional args.
 */
int ParseLaunchArguments(const int argc, char** argv, ADUC_LaunchArguments* launchArgs)
{
    int result = 0;
    memset(launchArgs, 0, sizeof(*launchArgs));

#if _ADU_DEBUG
    launchArgs->logLevel = ADUC_LOG_DEBUG;
#else
    launchArgs->logLevel = ADUC_LOG_INFO;
#endif

    launchArgs->argc = argc;
    launchArgs->argv = argv;

    while (result == 0)
    {
        // clang-format off
        static struct option long_options[] =
        {
            { "version",                       no_argument,       0, 'v' },
            { "enable-iothub-tracing",         no_argument,       0, 'e' },
            { "health-check",                  no_argument,       0, 'h' },
            { "log-level",                     required_argument, 0, 'l' },
            { "connection-string",             required_argument, 0, 'c' },
            { "register-extension",            required_argument, 0, 'E' },
            { "extension-type",                required_argument, 0, 't' },
            { "extension-id",                  required_argument, 0, 'i' },
            { "run-as-owner",                  no_argument,       0, 'a' },
            { "command",                       required_argument, 0, 'C' },
            { 0, 0, 0, 0 }
        };
        // clang-format on

        /* getopt_long stores the option index here. */
        int option_index = 0;

        int option = getopt_long(
            argc,
            argv,
            STOP_PARSE_ON_NONOPTION_ARG RET_COLON_FOR_MISSING_OPTIONARG "avehcu:l:d:n:E:t:i:C:",
            long_options,
            &option_index);

        /* Detect the end of the options. */
        if (option == -1)
        {
            break;
        }

        switch (option)
        {
        case 'h':
            launchArgs->healthCheckOnly = true;
            break;

        case 'e':
            launchArgs->iotHubTracingEnabled = true;
            break;

        case 'l':
        {
            unsigned int logLevel = 0;
            bool ret = atoui(optarg, &logLevel);
            if (!ret || logLevel < ADUC_LOG_DEBUG || logLevel > ADUC_LOG_ERROR)
            {
                puts("Invalid log level after '--log-level' or '-l' option. Expected value: 0-3.");
                result = -1;
            }
            else
            {
                launchArgs->logLevel = (ADUC_LOG_SEVERITY)logLevel;
            }

            break;
        }

        case 'v':
            launchArgs->showVersion = true;
            break;

        case 'c':
            launchArgs->connectionString = optarg;
            break;

        case 'C':
            launchArgs->ipcCommand = optarg;
            break;

        case 'E':
            launchArgs->extensionFilePath = optarg;
            break;

        case 't':
            launchArgs->extensionRegistrationType = GetRegistrationTypeFromArg(optarg);
            break;

        case 'i':
            launchArgs->extensionId = optarg;
            break;

        case ':':
            switch (optopt)
            {
            case 'c':
                puts("Missing connection string after '--connection-string' or '-c' option.");
                break;
            case 'l':
                puts("Invalid log level after '--log-level' or '-l' option. Expected value: 0-3.");
                break;
            default:
                printf("Missing an option value after -%c.\n", optopt);
                break;
            }
            result = -1;
            break;

        case '?':
            if (optopt)
            {
                printf("Unsupported short argument -%c.\n", optopt);
            }
            else
            {
                printf(
                    "Unsupported option '%s'. Try preceding with -- to separate options and additional args.\n",
                    argv[optind - 1]);
            }
            result = -1;
            break;
        default:
            printf("Unknown argument.");
            result = -1;
        }
    }

    if (result != -1 && optind < argc)
    {
        if (launchArgs->connectionString == NULL)
        {
            // Assuming first unknown option not starting with '-' is a connection string.
            for (int i = optind; i < argc; ++i)
            {
                if (argv[i][0] != '-')
                {
                    launchArgs->connectionString = argv[i];
                    ++optind;
                    break;
                }
            }
        }

        if (optind < argc)
        {
            // Still have unknown arg(s) on the end so return the index in argv where these start.
            result = optind;
        }
    }

    if (result == 0 && launchArgs->connectionString)
    {
        ADUC_StringUtils_Trim(launchArgs->connectionString);
    }

    return result;
}

/**
 * @brief Sets the Diagnostic DeviceName for creating the device's diagnostic container
 * @param connectionString connectionString to extract the device-id and module-id from
 * @returns true on success; false on failure
 */
bool ADUC_SetDiagnosticsDeviceNameFromConnectionString(const char* connectionString)
{
    bool succeeded = false;

    char* deviceId = NULL;

    char* moduleId = NULL;

    if (!ConnectionStringUtils_GetDeviceIdFromConnectionString(connectionString, &deviceId))
    {
        goto done;
    }

    // Note: not all connection strings have a module-id
    ConnectionStringUtils_GetModuleIdFromConnectionString(connectionString, &moduleId);

    if (!DiagnosticsComponent_SetDeviceName(deviceId, moduleId))
    {
        goto done;
    }

    succeeded = true;

done:

    free(deviceId);
    free(moduleId);
    return succeeded;
}

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

/**
 * @brief Scans the connection string and returns the connection type related to the string
 * @details The connection string must use the valid, correct format for the DeviceId and/or the ModuleId
 * e.g.
 * "DeviceId=some-device-id;ModuleId=some-module-id;"
 * If the connection string contains the DeviceId it is an ADUC_ConnType_Device
 * If the connection string contains the DeviceId AND the ModuleId it is an ADUC_ConnType_Module
 * @param connectionString the connection string to scan
 * @returns the connection type for @p connectionString
 */
ADUC_ConnType GetConnTypeFromConnectionString(const char* connectionString)
{
    ADUC_ConnType result = ADUC_ConnType_NotSet;

    if (connectionString == NULL)
    {
        Log_Debug("Connection string passed to GetConnTypeFromConnectionString is NULL");
        return ADUC_ConnType_NotSet;
    }

    if (ConnectionStringUtils_DoesKeyExist(connectionString, "DeviceId"))
    {
        if (ConnectionStringUtils_DoesKeyExist(connectionString, "ModuleId"))
        {
            result = ADUC_ConnType_Module;
        }
        else
        {
            result = ADUC_ConnType_Device;
        }
    }
    else
    {
        Log_Debug("DeviceId not present in connection string.");
    }

    return result;
}

/**
 * @brief Get the Connection Info from connection string, if a connection string is provided in configuration file
 *
 * @return true if connection info can be obtained
 */
bool GetConnectionInfoFromConnectionString(ADUC_ConnectionInfo* info, const char* connectionString)
{
    bool succeeded = false;

    ADUC_ConfigInfo config;
    memset(&config, 0, sizeof(config));

    if (info == NULL)
    {
        goto done;
    }
    if (connectionString == NULL)
    {
        goto done;
    }

    memset(info, 0, sizeof(*info));

    char certificateString[8192];

    if (mallocAndStrcpy_s(&info->connectionString, connectionString) != 0)
    {
        goto done;
    }

    info->connType = GetConnTypeFromConnectionString(info->connectionString);

    if (info->connType == ADUC_ConnType_NotSet)
    {
        Log_Error("Connection string is invalid");
        goto done;
    }

    info->authType = ADUC_AuthType_SASToken;

    // Optional: The certificate string is needed for Edge Gateway connection.
    if (ADUC_ConfigInfo_Init(&config, ADUC_CONF_FILE_PATH) && config.edgegatewayCertPath != NULL)
    {
        if (!LoadBufferWithFileContents(config.edgegatewayCertPath, certificateString, ARRAY_SIZE(certificateString)))
        {
            Log_Error("Failed to read the certificate from path: %s", config.edgegatewayCertPath);
            goto done;
        }

        if (mallocAndStrcpy_s(&info->certificateString, certificateString) != 0)
        {
            Log_Error("Failed to copy certificate string.");
            goto done;
        }

        info->authType = ADUC_AuthType_NestedEdgeCert;
    }

    succeeded = true;

done:
    ADUC_ConfigInfo_UnInit(&config);
    return succeeded;
}

/**
 * @brief Get the Connection Info from Identity Service
 *
 * @return true if connection info can be obtained
 */
bool GetConnectionInfoFromIdentityService(ADUC_ConnectionInfo* info)
{
    bool succeeded = false;
    if (info == NULL)
    {
        goto done;
    }
    memset(info, 0, sizeof(*info));

    time_t expirySecsSinceEpoch = time(NULL) + EIS_TOKEN_EXPIRY_TIME_IN_SECONDS;

    EISUtilityResult eisProvisionResult =
        RequestConnectionStringFromEISWithExpiry(expirySecsSinceEpoch, EIS_PROVISIONING_TIMEOUT, info);

    if (eisProvisionResult.err != EISErr_Ok && eisProvisionResult.service != EISService_Utils)
    {
        Log_Info(
            "Failed to provision a connection string from eis, Failed with error %s on service %s",
            EISErr_ErrToString(eisProvisionResult.err),
            EISService_ServiceToString(eisProvisionResult.service));
        goto done;
    }

    succeeded = true;
done:

    return succeeded;
}

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

/**
 * @brief Gets the agent configuration information and loads it according to the provisioning scenario
 *
 * @param info the connection information that will be configured
 * @return true on success; false on failure
 */
bool GetAgentConfigInfo(ADUC_ConnectionInfo* info)
{
    bool success = false;
    ADUC_ConfigInfo config = {};
    if (info == NULL)
    {
        return false;
    }

    if (!ADUC_ConfigInfo_Init(&config, ADUC_CONF_FILE_PATH))
    {
        Log_Error("No connection string set from launch arguments or configuration file");
        goto done;
    }

    const ADUC_AgentInfo* agent = ADUC_ConfigInfo_GetAgent(&config, 0);
    if (agent == NULL)
    {
        Log_Error("ADUC_ConfigInfo_GetAgent failed to get the agent information.");
        goto done;
    }

    if (strcmp(agent->connectionType, "AIS") == 0)
    {
        if (!GetConnectionInfoFromIdentityService(info))
        {
            Log_Error("Failed to get connection information from AIS.");
            goto done;
        }
    }
    else if (strcmp(agent->connectionType, "string") == 0)
    {
        if (!GetConnectionInfoFromConnectionString(info, agent->connectionData))
        {
            goto done;
        }
    }
    else
    {
        Log_Error("The connection type %s is not supported", agent->connectionType);
        goto done;
    }

    success = true;

done:
    if (!success)
    {
        ADUC_ConnectionInfo_DeAlloc(info);
    }

    ADUC_ConfigInfo_UnInit(&config);

    return success;
}

/**
 * @brief Handles the startup of the agent
 * @details Provisions the connection string with the CLI or either
 * the Edge Identity Service or the configuration file
 * @param launchArgs CLI arguments passed to the client
 * @returns bool true on success.
 */
bool StartupAgent(const ADUC_LaunchArguments* launchArgs)
{
    bool succeeded = false;

    ADUC_ConnectionInfo info = {};

    if (!ADUC_D2C_Messaging_Init())
    {
        goto done;
    }

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
    {
        if (!GetAgentConfigInfo(&info))
        {
            goto done;
        }

        if (!ADUC_SetDiagnosticsDeviceNameFromConnectionString(info.connectionString))
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

    if (!ADUC_PnP_Components_Create(g_iotHubClientHandle, launchArgs->argc, launchArgs->argv))
    {
        Log_Error("ADUC_PnP_Components_Create failed");
        goto done;
    }

    ADUC_Result result;

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

    succeeded = true;

done:

    ADUC_ConnectionInfo_DeAlloc(&info);
    return succeeded;
}

/**
 * @brief Called at agent shutdown.
 */
void ShutdownAgent()
{
    Log_Info("Agent is shutting down with signal %d.", g_shutdownSignal);
    ADUC_D2C_Messaging_Uninit();
    UninitializeCommandListenerThread();
    ADUC_PnP_Components_Destroy();
    IoTHub_CommunicationManager_Deinit();
    DiagnosticsComponent_DestroyDeviceName();
    ADUC_Logging_Uninit();
    ExtensionManager_Uninit();
}

/**
 * @brief Called when a terminate (SIGINT, SIGTERM) signal is detected.
 *
 * @param sig Signal value.
 */
void OnShutdownSignal(int sig)
{
    // Main loop will break once this becomes true.
    g_shutdownSignal = sig;
}

/**
 * @brief Called when a restart (SIGUSR1) signal is detected.
 *
 * @param sig Signal value.
 */
void OnRestartSignal(int sig)
{
    // Note: Main loop will break once this becomes true. We rely on the 'Restart' setting in
    // deviceupdate-agent.service file to instruct systemd to restart the agent.
    Log_Info("Restart signal detect.");
    g_shutdownSignal = sig;
}

/**
 * @brief Sets effective user id as specified in du-config.json (agents[#].ranAs property),
 * and Sets effective group id to as ADUC_FILE_GROUP.
 *
 * This to ensure that the agent process is run with the intended privileges, and the resource that
 * created by the agent has the correct ownership.
 *
 * @return bool true on success.
 */
bool RunAsDesiredUser()
{
    bool success = false;
    ADUC_ConfigInfo config = {};
    if (!ADUC_ConfigInfo_Init(&config, ADUC_CONF_FILE_PATH))
    {
        Log_Error("Cannot read configuration file.");
        return false;
    }

    if (!PermissionUtils_SetProcessEffectiveGID(ADUC_FILE_GROUP))
    {
        Log_Error("Failed to set process effective group to '%s'. (errno:%d)", ADUC_FILE_GROUP, errno);
        goto done;
    }

    if (!PermissionUtils_SetProcessEffectiveUID(config.agents[0].runas))
    {
        Log_Error("Failed to set process effective user to '%s'. (errno:%d)", config.agents[0].runas, errno);
        goto done;
    }

    success = true;
done:
    ADUC_ConfigInfo_UnInit(&config);
    return success;
}

//
// Main.
//

/**
 * @brief Main method.
 *
 * @param argc Count of arguments in @p argv.
 * @param argv Arguments, first is the process name.
 * @return int Return value. 0 for succeeded.
 *
 * @note argv[0]: process name
 * @note argv[1]: connection string.
 * @note argv[2..n]: optional parameters for upper layer.
 */
int main(int argc, char** argv)
{
    InitializeModeledComponents();

    int ret = ParseLaunchArguments(argc, argv, &launchArgs);
    if (ret < 0)
    {
        return ret;
    }

    if (launchArgs.showVersion)
    {
        puts(ADUC_VERSION);
        return 0;
    }

    // Need to set ret and goto done after this to ensure proper shutdown and deinitialization.
    ADUC_Logging_Init(launchArgs.logLevel, "du-agent");

    // default to failure
    ret = 1;

    if (launchArgs.healthCheckOnly)
    {
        if (HealthCheck(&launchArgs))
        {
            ret = 0;
        }

        goto done;
    }

    if (launchArgs.extensionFilePath != NULL)
    {
        switch (launchArgs.extensionRegistrationType)
        {
        case ExtensionRegistrationType_None:
            Log_Error("Missing --extension-type argument.");
            goto done;

        case ExtensionRegistrationType_UpdateContentHandler:
            if (launchArgs.extensionId == NULL)
            {
                Log_Error("Missing --extension-id argument.");
                goto done;
            }

            if (RegisterUpdateContentHandler(launchArgs.extensionId, launchArgs.extensionFilePath))
            {
                ret = 0;
            }

            goto done;

        case ExtensionRegistrationType_ComponentEnumerator:
            if (RegisterComponentEnumeratorExtension(launchArgs.extensionFilePath))
            {
                ret = 0;
            }

            goto done;

        case ExtensionRegistrationType_ContentDownloadHandler:
            if (RegisterContentDownloaderExtension(launchArgs.extensionFilePath))
            {
                ret = 0;
            }

            goto done;

        case ExtensionRegistrationType_DownloadHandler:
            if (launchArgs.extensionId == NULL)
            {
                Log_Error("Missing --extension-id argument.");
                goto done;
            }

            if (RegisterDownloadHandler(launchArgs.extensionId, launchArgs.extensionFilePath))
            {
                ret = 0;
            }

            goto done;

        default:
            Log_Error("Unknown ExtensionRegistrationType: %d", launchArgs.extensionRegistrationType);
            goto done;
        }
    }

    // This instance of an agent is launched for sending command to the main agent process.
    if (launchArgs.ipcCommand != NULL)
    {
        if (SendCommand(launchArgs.ipcCommand))
        {
            ret = 0;
        }

        goto done;
    }

    // Switch to specified agent.runas user.
    // Note: it's important that we do this only when we're not performing any
    // high-privileged tasks, such as, registering agent's extension(s).
    if (!RunAsDesiredUser())
    {
        goto done;
    }

    Log_Info("Agent (%s; %s) starting.", ADUC_PLATFORM_LAYER, ADUC_VERSION);
#ifdef ADUC_GIT_INFO
    Log_Info("Git Info: %s", ADUC_GIT_INFO);
#endif
    Log_Info(
        "Supported Update Manifest version: min: %d, max: %d",
        SUPPORTED_UPDATE_MANIFEST_VERSION_MIN,
        SUPPORTED_UPDATE_MANIFEST_VERSION_MAX);

    bool healthy = HealthCheck(&launchArgs);
    if (launchArgs.healthCheckOnly || !healthy)
    {
        if (healthy)
        {
            Log_Info("Agent is healthy.");
            ret = 0;
        }
        else
        {
            Log_Error("Agent health check failed.");
        }

        goto done;
    }

    // Ensure that the ADU data folder exists.
    // Normally, ADUC_DATA_FOLDER is created by install script.
    // However, if we want to run the Agent without installing the package, we need to manually
    // create the folder. (e.g. when running UTs in build pipelines, side-loading for testing, etc.)
    int dir_result = ADUC_SystemUtils_MkDirRecursiveDefault(ADUC_DATA_FOLDER);
    if (dir_result != 0)
    {
        Log_Error("Cannot create data folder.");
        goto done;
    }

    //
    // Catch ctrl-C and shutdown signals so we do a best effort of cleanup.
    //
    signal(SIGINT, OnShutdownSignal);
    signal(SIGTERM, OnShutdownSignal);

    //
    // Catch restart (SIGUSR1) signal raised by a workflow.
    //
    signal(SIGUSR1, OnRestartSignal);

    if (!StartupAgent(&launchArgs))
    {
        goto done;
    }

    //
    // Main Loop
    //

    Log_Info("Agent running.");
    while (g_shutdownSignal == 0)
    {
        // If any components have requested a DoWork callback, regularly call it.
        for (unsigned index = 0; index < ARRAY_SIZE(componentList); ++index)
        {
            PnPComponentEntry* entry = componentList + index;

            if (entry->DoWork != NULL)
            {
                entry->DoWork(entry->Context);
            }
        }

        IoTHub_CommunicationManager_DoWork(&g_iotHubClientHandle);
        ADUC_D2C_Messaging_DoWork();

        // NOTE: When using low level samples (iothub_ll_*), the IoTHubDeviceClient_LL_DoWork
        // function must be called regularly (eg. every 100 milliseconds) for the IoT device client to work properly.
        // See: https://github.com/Azure/azure-iot-sdk-c/tree/master/iothub_client/samples
        // NOTE: For this example the above has been wrapped to support module and device client methods using
        // the client_handle_helper.h function ClientHandle_DoWork()

        ThreadAPI_Sleep(100);
    };

    ret = 0; // Success.

done:
    Log_Info("Agent exited with code %d", ret);

    ShutdownAgent();

    return ret;
}
