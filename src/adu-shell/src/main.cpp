/**
 * @file main.cpp
 * @brief Implements the main code for the ADU Shell.
 *
 * @copyright Copyright (c) 2019, Microsoft Corporation.
 */
#include <getopt.h>
#include <string.h>
#include <unistd.h> // for geteuid and setuid.
#include <unordered_map>
#include <vector>

#include "aduc/c_utils.h"
#include "aduc/logging.h"
#include "aduc/process_utils.hpp"
#include "aduc/string_utils.hpp"
#include "aduc/system_utils.h"

#include "adushell.hpp"
#include "adushell_const.hpp"
#include "common_tasks.hpp"

namespace CommonTasks = Adu::Shell::Tasks::Common;

#ifdef ADUSHELL_APT
#    include "aptget_tasks.h"
namespace AptGetTasks = Adu::Shell::Tasks::AptGet;
#endif

#ifdef ADUSHELL_SWUPDATE
#    include "swupdate_tasks.hpp"
namespace SWUpdateTasks = Adu::Shell::Tasks::SWUpdate;
#endif

#ifdef ADUSHELL_PVCONTROL
#    include "pvcontrol_tasks.hpp"
namespace PVControlTasks = Adu::Shell::Tasks::PVControl;
#endif

namespace adushconst = Adu::Shell::Const;

/**
 * @brief Parse command-line arguments.
 * @param argc arguments count.
 * @param argv arguments array.
 * @param launchArgs a struct to store the parsed arguments.
 *
 * @return 0 if succeeded.
 */
int ParseLaunchArguments(const int argc, char** argv, ADUShell_LaunchArguments* launchArgs)
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

        // "--version"           |   Show adu-shell version number.
        //
        // "--update-type"       |   An ADU Update Type.  
        //                             e.g., "microsoft/apt", "microsoft/swupdate", "pantacor/pvcontrol", "common".
        //
        // "--update-action"     |   An action to perform.
        //                             e.g., "initialize", "download", "install", "apply", "cancel", "rollback", "reboot", "getstatus".
        //
        // "--target-data"       |   A string contains data for a target command. 
        //                             e.g., for microsoft/apt download action, this is a single-quoted string
        //                             contains space-delimited list of package names.
        // 
        // "--target-options"    |   Additional options for a target command.
        //
        // "--target-log-folder" |   Some target command requires specific logs storage.
        //
        // "--log-level"         |   Log verbosity level.
        //
        static struct option long_options[] =
        {
            { "version",           no_argument,       nullptr, 'v' },
            { "update-type",       required_argument, nullptr, 't' },
            { "update-action",     required_argument, nullptr, 'a' },
            { "target-data",       required_argument, nullptr, 'd' },
            { "target-options",    required_argument, nullptr, 'o' },
            { "target-log-folder", required_argument, nullptr, 'f' },
            { "log-level",         required_argument, nullptr, 'l' },
            { nullptr, 0, nullptr, 0 }
        };

        // clang-format on

        /* getopt_long stores the option index here. */
        int option_index = 0;
        int option = getopt_long(argc, argv, "vt:a:d:o:f:l:", long_options, &option_index);

        /* Detect the end of the options. */
        if (option == -1)
        {
            break;
        }

        switch (option)
        {
        case 'v':
            launchArgs->showVersion = true;
            break;

        case 't':
            launchArgs->updateType = optarg;
            break;

        case 'a':
            launchArgs->updateAction = optarg;
            launchArgs->action = ADUShellActionFromString(launchArgs->updateAction);
            break;

        case 'd':
            launchArgs->targetData = optarg;
            break;

        case 'o':
            launchArgs->targetOptions = optarg;
            break;

        case 'f':
            launchArgs->logFile = optarg;
            break;

        case 'l':
        {
            char* endptr;
            errno = 0; /* To distinguish success/failure after call */
            int64_t logLevel = strtol(optarg, &endptr, 10);
            if (errno != 0 || endptr == optarg || logLevel < ADUC_LOG_DEBUG || logLevel >= ADUC_LOG_ERROR)
            {
                printf("Invalid log level after '--log-level' or '-l' option. Expected value: 0-3.");
                result = -1;
            }
            else
            {
                launchArgs->logLevel = static_cast<ADUC_LOG_SEVERITY>(logLevel);
            }

            break;
        }

        case '?':
            switch (optopt)
            {
            case 't':
                printf("Missing an Update Type string after '--update-type' or '-t' option.");
                break;
            case 'd':
                printf("Missing a target data string after '--target-data' or '-d' option. Expected quoted string.");
                break;
            case 'o':
                printf(
                    "Missing a target options string after '--target-options' or '-o' option. Expected quoted string.");
                break;
            case 'l':
                printf("Invalid log level after '--log-level' or '-l' option. Expected value: 0-3.");
                break;
            case 'f':
                printf("Missing a log folder path after '--target-log-folder' or '-f' option.");
                break;
            default:
                printf("Missing an option value after -%c.\n", optopt);
                break;
            }
            result = -1;
            break;

        default:
            printf("Ignoring unknown argument: character code %d", option);
        }
    }

    if (launchArgs->updateType == nullptr)
    {
        printf("Missing --update-type option.\n");
        result = -1;
    }

    if (launchArgs->updateAction == nullptr)
    {
        printf("Missing --update-action option.\n");
        result = -1;
    }

    return result;
}

std::string GetFormattedCommandline(const std::string& command, const std::vector<std::string>& args)
{
    std::stringstream output;
    output << command << ' ';
    for (const std::string& arg : args)
    {
        output << arg << ' ';
    }
    return output.str();
}

// TODO (Nox): 31082410: Revisit how to merge and display logs from adu-shell
//            in adu-agent journalctl log w/o flooding adu log file.
void ShowChildProcessLogs(const std::string& output)
{
    if (!output.empty())
    {
        std::stringstream ss(output);
        std::string token;
        Log_Info("########## Begin Child's Logs ##########");
        while (std::getline(ss, token, '\n'))
        {
            Log_Info("#  %s", token.c_str());
        }
        Log_Info("########## End Child's Logs ##########");
    }
}

/**
 * @brief Starts a child process for task(s) for a given update actions.
 */
int ADUShell_Dowork(const ADUShell_LaunchArguments& launchArgs)
{
    ADUShellTaskResult taskResult;

    try
    {
        const std::unordered_map<std::string, ADUShellTaskFuncType> actionMap = {
            { adushconst::update_type_common, CommonTasks::DoCommonTask },
#ifdef ADUSHELL_APT
            { adushconst::update_type_microsoft_apt, AptGetTasks::DoAptGetTask },
#endif
#ifdef ADUSHELL_SWUPDATE
            { adushconst::update_type_microsoft_swupdate, SWUpdateTasks::DoSWUpdateTask },
#endif
#ifdef ADUSHELL_PVCONTROL
            { adushconst::update_type_pantacor_pvcontrol, PVControlTasks::DoPVControlTask }
#endif
        };

        ADUShellTaskFuncType task = actionMap.at(std::string(launchArgs.updateType));
        taskResult = task(launchArgs);
    }
    catch (...)
    {
        Log_Error("Unknown update type: '%s'", launchArgs.updateType);
        taskResult.SetExitStatus(ADUSHELL_EXIT_UNSUPPORTED);
    }

    ShowChildProcessLogs(taskResult.Output());

    return taskResult.ExitStatus();
}

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
    ADUShell_LaunchArguments launchArgs;

    int ret = ParseLaunchArguments(argc, argv, &launchArgs);
    if (ret != 0)
    {
        return ret;
    }

    if (launchArgs.showVersion)
    {
        printf(ADUC_VERSION);
        return 0;
    }

    ADUC_Logging_Init(launchArgs.logLevel);

    Log_Debug("Update type: %s", launchArgs.updateType);
    Log_Debug("Update action: %s", launchArgs.updateAction);
    Log_Debug("Target data: %s", launchArgs.targetData);
    Log_Debug("Target options: %s", launchArgs.targetOptions);
    Log_Debug("Log level: %d", launchArgs.logLevel);

    // Run as 'root'.
    // Note: this requires the file owner to be 'root'.
    int defaultUserId = getuid();
    int effectiveUserId = geteuid();

    ret = setuid(effectiveUserId);
    if (ret == 0)
    {
        Log_Info("Run as uid(%d), defaultUid(%d), effectiveUid(%d)", getuid(), defaultUserId, effectiveUserId);
        return ADUShell_Dowork(launchArgs);
    }

    Log_Error("Cannot set user identity. (code: %d, errno: %d)", ret, errno);
    return ret;
}
