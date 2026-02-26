/**
 * @file mock_process_utils.h
 * @brief Test helpers for controlling the mock ADUC_LaunchChildProcess.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef MOCK_PROCESS_UTILS_H
#define MOCK_PROCESS_UTILS_H

#include <string>
#include <vector>

struct MockLaunchChildProcessState
{
    int exitCode = 0;
    std::string output;
    std::string fileToCreate; // if non-empty, mock will create this file
    std::string fileContent; // content to write to fileToCreate
    bool wasCalled = false;
    std::string lastCommand;
    std::vector<std::string> lastArgs;
};

void MockLaunchChildProcess_Reset();
void MockLaunchChildProcess_SetExitCode(int exitCode);
void MockLaunchChildProcess_SetOutput(const std::string& output);
void MockLaunchChildProcess_SetFileToCreate(const std::string& path, const std::string& content);
bool MockLaunchChildProcess_WasCalled();
std::string MockLaunchChildProcess_GetCommand();
std::vector<std::string> MockLaunchChildProcess_GetArgs();

#endif // MOCK_PROCESS_UTILS_H
