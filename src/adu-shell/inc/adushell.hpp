/**
 * @file adushell.hpp
 * @brief Private header for ADU Shell types and helper functions.
 * @copyright Copyright (c) 2020, Microsoft Corp.
 */
#ifndef ADU_SHELL_HPP
#define ADU_SHELL_HPP

#include "aduc/logging.h"
#include "adushell_action.hpp"

#include <functional>
#include <string>
#include <vector>

/**
 * @brief ADU Shell launch arguments.
 */
typedef struct tagADUShell_LaunchArguments
{
    int argc; /**< Size of argv */
    char** argv; /**< Command-line arguments */
    ADUC_LOG_SEVERITY logLevel; /**< Log level */
    char* updateType; /**< And ADU Update type */
    char* updateAction; /**< An ADU Update action */
    ADUShellAction action;
    char* targetData; /**< Data to pass to target command */
    char* targetOptions; /**< Additional options to pass to target command */
    char* logFile; /**< Custom log file path */
    bool showVersion; /**< Show an agent version */
} ADUShell_LaunchArguments;

/**
 * @brief A exit code from an ADUShell child process.
 * We're using EXIT_SUCCESS (0) for success case and EXIT_FAILURE (1) for general errors.
 */
#define ADUSHELL_EXIT_UNSUPPORTED 3

/**
 * @brief A result from ADUShell task.
 */
class ADUShellTaskResult
{
public:
    ADUShellTaskResult() = default;
    ~ADUShellTaskResult() = default;

    const int ExitStatus() const
    {
        return _exitStatus;
    }

    void SetExitStatus(int status)
    {
        _exitStatus = status;
    }

    std::string& Output()
    {
        return _output;
    }

private:
    int _exitStatus{ EXIT_SUCCESS }; /**< An exit code from child process. Default is EXIT_SUCCESS */
    std::string _output; /**< A string from child process' standard output stream */
};

/**
 * @brief Returns a string that concatenate a command and all arguments.
 * 
 * @param command A command line to run.
 * @param args An argument list.
 * 
 * @return A concatenated, space delimited string. 
 */
std::string GetFormattedCommandline(const std::string& command, const std::vector<std::string>& args);

using ADUShellTaskFuncType = std::function<ADUShellTaskResult(const ADUShell_LaunchArguments&)>;

#endif // ADU_SHELL_HPP