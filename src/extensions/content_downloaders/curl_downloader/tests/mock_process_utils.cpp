/**
 * @file mock_process_utils.cpp
 * @brief Link-time mock for ADUC_LaunchChildProcess used in curl_content_downloader tests.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "mock_process_utils.h"

#include <string>
#include <vector>
#include <cstdio>

// Global mock state – tests set these before calling Download_curl.
static MockLaunchChildProcessState s_mockState = {};

void MockLaunchChildProcess_Reset()
{
    s_mockState = {};
}

void MockLaunchChildProcess_SetExitCode(int exitCode)
{
    s_mockState.exitCode = exitCode;
}

void MockLaunchChildProcess_SetOutput(const std::string& output)
{
    s_mockState.output = output;
}

void MockLaunchChildProcess_SetFileToCreate(const std::string& path, const std::string& content)
{
    s_mockState.fileToCreate = path;
    s_mockState.fileContent = content;
}

bool MockLaunchChildProcess_WasCalled()
{
    return s_mockState.wasCalled;
}

std::string MockLaunchChildProcess_GetCommand()
{
    return s_mockState.lastCommand;
}

std::vector<std::string> MockLaunchChildProcess_GetArgs()
{
    return s_mockState.lastArgs;
}

// These must match the signatures in process_utils.hpp exactly.
// We define them here so the linker picks up our mock instead of the real one.
int ADUC_LaunchChildProcess(const std::string& command, std::vector<std::string> args, std::string& output)
{
    s_mockState.wasCalled = true;
    s_mockState.lastCommand = command;
    s_mockState.lastArgs = args;
    output = s_mockState.output;

    // If configured to create a file (simulating curl download), do so.
    if (!s_mockState.fileToCreate.empty())
    {
        FILE* f = fopen(s_mockState.fileToCreate.c_str(), "wb");
        if (f != nullptr)
        {
            fwrite(s_mockState.fileContent.data(), 1, s_mockState.fileContent.size(), f);
            fclose(f);
        }
    }

    return s_mockState.exitCode;
}

// Provide the vector<string> overload too, to satisfy the linker if needed.
int ADUC_LaunchChildProcess(
    const std::string& command, std::vector<std::string> args, std::vector<std::string>& output)
{
    std::string singleOutput;
    int ret = ADUC_LaunchChildProcess(command, args, singleOutput);
    if (!singleOutput.empty())
    {
        output.push_back(singleOutput);
    }
    return ret;
}
