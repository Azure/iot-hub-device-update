/**
 * @file script_handler_v3.c
 * @brief Script Handler v3 - Azure Device Update Step Handler SDK Example
 *
 * This demonstrates a real, functioning content handler built using the SDK.
 * It's a simplified version of the existing script handler that shows:
 * - How to implement the ContentHandler interface
 * - How to use SDK result types and utilities
 * - How to process workflow data and files
 * - How to execute scripts for each update phase
 *
 * @copyright Copyright (C) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See LICENSE file in the project root for license information.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

// SDK includes
#include "aduc/result.h"
#include "aduc/workflow_data.h"
#include "script_utils.h"

#define HANDLER_NAME "script_handler_v3"
#define HANDLER_VERSION "1.0.0"

/**
 * @brief Script Handler context structure
 */
typedef struct
{
    char* workFolder;
    char* scriptFilePath;
    char handlerName[64];
} ScriptHandlerContext;

// Forward declarations
static ADUC_Result_t ScriptHandler_Download(const ADUC_WorkflowData* workflowData, ADUC_ResultDetails* result);
static ADUC_Result_t ScriptHandler_Backup(const ADUC_WorkflowData* workflowData, ADUC_ResultDetails* result);
static ADUC_Result_t ScriptHandler_Install(const ADUC_WorkflowData* workflowData, ADUC_ResultDetails* result);
static ADUC_Result_t ScriptHandler_Apply(const ADUC_WorkflowData* workflowData, ADUC_ResultDetails* result);
static ADUC_Result_t ScriptHandler_Restore(const ADUC_WorkflowData* workflowData, ADUC_ResultDetails* result);
static ADUC_Result_t ScriptHandler_Cancel(const ADUC_WorkflowData* workflowData, ADUC_ResultDetails* result);
static ADUC_Result_t ScriptHandler_IsInstalled(const ADUC_WorkflowData* workflowData, ADUC_ResultDetails* result);

/**
 * @brief Log a message using the workflow data logger
 */
static void LogMessage(const ADUC_WorkflowData* workflowData, ADUC_LogLevel level, const char* message)
{
    // For now, just use printf since we don't have the logger callback in this simplified version
    const char* levelStr = "INFO";
    switch (level)
    {
        case ADUC_LOG_ERROR: levelStr = "ERROR"; break;
        case ADUC_LOG_WARN: levelStr = "WARN"; break;
        case ADUC_LOG_INFO: levelStr = "INFO"; break;
        case ADUC_LOG_DEBUG: levelStr = "DEBUG"; break;
    }
    printf("[%s] %s: %s\n", levelStr, HANDLER_NAME, message);
}

/**
 * @brief Execute a script with the given action
 */
static ADUC_Result_t ExecuteScript(const ADUC_WorkflowData* workflowData, const char* action, ADUC_ResultDetails* result)
{
    char logMessage[256];
    char command[512];
    int exitCode;

    if (!workflowData || !action || !result)
    {
        ADUC_Result_SetFailure(result, ADUC_Result_Failure_InvalidArgument, "Invalid arguments");
        return ADUC_Result_Failure_InvalidArgument;
    }

    const char* scriptPath = GetScriptFilePath(workflowData);
    if (!scriptPath)
    {
        ADUC_Result_SetFailure(result, ADUC_Result_Failure_FileNotFound, "Script file not found");
        return ADUC_Result_Failure_FileNotFound;
    }

    // Log the action we're performing
    snprintf(logMessage, sizeof(logMessage), "Executing script action: %s", action);
    LogMessage(workflowData, ADUC_LOG_INFO, logMessage);

    // Build the command
    snprintf(command, sizeof(command), "\"%s\" %s", scriptPath, action);

    // Report status
    if (workflowData->statusCallback)
    {
        snprintf(logMessage, sizeof(logMessage), "Running %s action", action);
        workflowData->statusCallback(workflowData->statusContext, ADUC_UpdateState_InstallStarted, logMessage);
    }

    // Execute the command
    LogMessage(workflowData, ADUC_LOG_DEBUG, command);
    exitCode = system(command);

    if (exitCode == 0)
    {
        snprintf(logMessage, sizeof(logMessage), "Script action %s completed successfully", action);
        LogMessage(workflowData, ADUC_LOG_INFO, logMessage);
        ADUC_Result_SetSuccess(result, logMessage);
        return ADUC_Result_Success;
    }
    else
    {
        snprintf(logMessage, sizeof(logMessage), "Script action %s failed with exit code %d", action, exitCode);
        LogMessage(workflowData, ADUC_LOG_ERROR, logMessage);
        ADUC_Result_SetFailure(result, ADUC_Result_Failure_InstallFailed, logMessage);
        return ADUC_Result_Failure_InstallFailed;
    }
}

/**
 * @brief Download phase implementation
 */
static ADUC_Result_t ScriptHandler_Download(const ADUC_WorkflowData* workflowData, ADUC_ResultDetails* result)
{
    LogMessage(workflowData, ADUC_LOG_INFO, "Starting Download phase");

    // For script handler, download typically means:
    // 1. Download script files to work folder
    // 2. Make them executable

    ADUC_Result_t downloadResult = DownloadScriptFiles(workflowData, result);
    if (downloadResult != ADUC_Result_Success)
    {
        return downloadResult;
    }

    // Execute script download action if available
    return ExecuteScript(workflowData, "download", result);
}

/**
 * @brief Backup phase implementation
 */
static ADUC_Result_t ScriptHandler_Backup(const ADUC_WorkflowData* workflowData, ADUC_ResultDetails* result)
{
    LogMessage(workflowData, ADUC_LOG_INFO, "Starting Backup phase");
    return ExecuteScript(workflowData, "backup", result);
}

/**
 * @brief Install phase implementation
 */
static ADUC_Result_t ScriptHandler_Install(const ADUC_WorkflowData* workflowData, ADUC_ResultDetails* result)
{
    LogMessage(workflowData, ADUC_LOG_INFO, "Starting Install phase");
    return ExecuteScript(workflowData, "install", result);
}

/**
 * @brief Apply phase implementation
 */
static ADUC_Result_t ScriptHandler_Apply(const ADUC_WorkflowData* workflowData, ADUC_ResultDetails* result)
{
    LogMessage(workflowData, ADUC_LOG_INFO, "Starting Apply phase");
    return ExecuteScript(workflowData, "apply", result);
}

/**
 * @brief Restore phase implementation
 */
static ADUC_Result_t ScriptHandler_Restore(const ADUC_WorkflowData* workflowData, ADUC_ResultDetails* result)
{
    LogMessage(workflowData, ADUC_LOG_INFO, "Starting Restore phase");
    return ExecuteScript(workflowData, "restore", result);
}

/**
 * @brief Cancel phase implementation
 */
static ADUC_Result_t ScriptHandler_Cancel(const ADUC_WorkflowData* workflowData, ADUC_ResultDetails* result)
{
    LogMessage(workflowData, ADUC_LOG_INFO, "Starting Cancel phase");
    return ExecuteScript(workflowData, "cancel", result);
}

/**
 * @brief IsInstalled check implementation
 */
static ADUC_Result_t ScriptHandler_IsInstalled(const ADUC_WorkflowData* workflowData, ADUC_ResultDetails* result)
{
    LogMessage(workflowData, ADUC_LOG_INFO, "Checking IsInstalled");
    return ExecuteScript(workflowData, "isInstalled", result);
}

/**
 * @brief Simple interface for testing script execution
 */
ADUC_Result_t ScriptHandlerV3_ProcessAction(const ADUC_WorkflowData* workflowData, const char* action, ADUC_ResultDetails* result)
{
    if (!workflowData || !action || !result)
    {
        return ADUC_Result_Failure_InvalidArgument;
    }

    if (strcmp(action, "download") == 0)
    {
        return ScriptHandler_Download(workflowData, result);
    }
    else if (strcmp(action, "backup") == 0)
    {
        return ScriptHandler_Backup(workflowData, result);
    }
    else if (strcmp(action, "install") == 0)
    {
        return ScriptHandler_Install(workflowData, result);
    }
    else if (strcmp(action, "apply") == 0)
    {
        return ScriptHandler_Apply(workflowData, result);
    }
    else if (strcmp(action, "restore") == 0)
    {
        return ScriptHandler_Restore(workflowData, result);
    }
    else if (strcmp(action, "cancel") == 0)
    {
        return ScriptHandler_Cancel(workflowData, result);
    }
    else if (strcmp(action, "isInstalled") == 0)
    {
        return ScriptHandler_IsInstalled(workflowData, result);
    }
    else
    {
        ADUC_Result_SetFailure(result, ADUC_Result_Failure_InvalidArgument, "Unknown action");
        return ADUC_Result_Failure_InvalidArgument;
    }
}
