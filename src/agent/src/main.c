/**
 * @file main.c
 * @brief Implements the main code for the Device Update Agent.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/adu_core_export_helpers.h"
#include "aduc/adu_types.h"
#include "aduc/agent_workflow.h"
#include "aduc/agent-workflow-module.h"
#include "aduc/c_utils.h"
#include "aduc/command_helper.h"
#include "aduc/config_utils.h"
#include "aduc/connection_string_utils.h"
#include "aduc/extension_manager.h"
#include "aduc/extension_utils.h"
#include "aduc/health_management.h"
#include "aduc/https_proxy_utils.h"
#include "aduc/logging.h"
#include "aduc/permission_utils.h"
#include "aduc/string_c_utils.h"
#include "aduc/system_utils.h"
#include <azure_c_shared_utility/shared_util_options.h>
#include <azure_c_shared_utility/threadapi.h> // ThreadAPI_Sleep
#include <ctype.h>
#include <getopt.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h> // strtol
#include <sys/stat.h>
#include <unistd.h>
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

ADUC_LaunchArguments launchArgs;

/**
 * @brief Determines if we're shutting down.
 *
 * Value indicates the shutdown signal if shutdown requested.
 *
 * Do not shutdown = 0; SIGINT = 2; SIGTERM = 15;
 */
static int g_shutdownSignal = 0;

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

    launchArgs->configFolder = ADUC_CONF_FOLDER;

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
            { "config-folder",                 required_argument, 0, 'F' },
            { 0, 0, 0, 0 }
        };
        // clang-format on

        /* getopt_long stores the option index here. */
        int option_index = 0;

        int option = getopt_long(
            argc,
            argv,
            STOP_PARSE_ON_NONOPTION_ARG RET_COLON_FOR_MISSING_OPTIONARG "avehcu:l:d:n:E:t:i:C:F:",
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

        case 'F':
            launchArgs->configFolder = optarg;
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
            case 'F':
                puts("Missing folder path after '--config-folder' or '-F' option.");
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


// TODO (nox-msft) - static ADUC_DeviceUpdateAgentModule g_agentWorkflowModules[3];

int InitAgentWorkflowModules()
{
    // TODO (nox-msft) - Intialize all agent workflow modules
    return 0;
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

    InitAgentWorkflowModules();



    succeeded = true;

    return succeeded;
}

/**
 * @brief Called at agent shutdown.
 */
void ShutdownAgent()
{
    Log_Info("Agent is shutting down with signal %d.", g_shutdownSignal);
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
 * @remark This function requires that the ADUC_ConfigInfo singleton has been initialized.
 *
 * @return bool true on success.
 */
bool RunAsDesiredUser()
{
    bool success = false;
    const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
    if (config == NULL)
    {
        Log_Error("ADUC_ConfigInfo_GetInstance failed.");
        return false;
    }

    if (!PermissionUtils_SetProcessEffectiveGID(ADUC_FILE_GROUP))
    {
        Log_Error("Failed to set process effective group to '%s'. (errno:%d)", ADUC_FILE_GROUP, errno);
        goto done;
    }

    if (!PermissionUtils_SetProcessEffectiveUID(config->agents[0].runas))
    {
        Log_Error("Failed to set process effective user to '%s'. (errno:%d)", config->agents[0].runas, errno);
        goto done;
    }

    success = true;
done:
    ADUC_ConfigInfo_ReleaseInstance(config);

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

    setenv(ADUC_CONFIG_FOLDER_ENV, launchArgs.configFolder, 1);

    const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
    if (config == NULL)
    {
        Log_Error("Cannot initialize config info.");
        return -1;
    }

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

        // TODO (nox-msft) - Call agent workflow modules' DOWork()

        ThreadAPI_Sleep(100);
    };

    ret = 0; // Success.

done:
    Log_Info("Agent exited with code %d", ret);

    ShutdownAgent();

    ADUC_ConfigInfo_ReleaseInstance(config);

    return ret;
}
