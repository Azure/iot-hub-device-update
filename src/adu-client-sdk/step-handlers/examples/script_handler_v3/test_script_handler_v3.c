/**
 * @file test_script_handler_v3.c
 * @brief Test program for Script Handler v3
 *
 * @copyright Copyright (C) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See LICENSE file in the project root for license information.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include "aduc/result.h"
#include "aduc/workflow_data.h"

// Function prototype from script handler v3
ADUC_Result_t ScriptHandlerV3_ProcessAction(const ADUC_WorkflowData* workflowData, const char* action, ADUC_ResultDetails* result);

/**
 * @brief Create a mock workflow data for testing
 */
static ADUC_WorkflowData* CreateMockWorkflowData(const char* scriptPath)
{
    ADUC_WorkflowData* workflowData = (ADUC_WorkflowData*)calloc(1, sizeof(ADUC_WorkflowData));
    if (!workflowData)
    {
        return NULL;
    }

    // Set basic properties
    workflowData->workflowId = "test-workflow-123";
    workflowData->workFolder = "/tmp/adu-test";
    workflowData->updateManifest = "{}";
    workflowData->handlerProperties = "{}";

    // Create a single file entry
    workflowData->files = (ADUC_FileInfo*)calloc(1, sizeof(ADUC_FileInfo));
    if (!workflowData->files)
    {
        free(workflowData);
        return NULL;
    }

    workflowData->fileCount = 1;
    workflowData->files[0].fileName = "test-script.sh";
    workflowData->files[0].filePath = scriptPath;
    workflowData->files[0].fileSize = 0;
    workflowData->files[0].hash = "mock-hash";
    workflowData->files[0].hashAlgorithm = "SHA256";

    return workflowData;
}

/**
 * @brief Free mock workflow data
 */
static void FreeMockWorkflowData(ADUC_WorkflowData* workflowData)
{
    if (workflowData)
    {
        free(workflowData->files);
        free(workflowData);
    }
}

/**
 * @brief Create a test script
 */
static int CreateTestScript(const char* scriptPath)
{
    FILE* fp = fopen(scriptPath, "w");
    if (!fp)
    {
        printf("Failed to create test script at %s\n", scriptPath);
        return -1;
    }

    fprintf(fp, "#!/bin/bash\n");
    fprintf(fp, "# Test script for ADU Script Handler v3\n");
    fprintf(fp, "\n");
    fprintf(fp, "ACTION=\"$1\"\n");
    fprintf(fp, "\n");
    fprintf(fp, "echo \"[TEST-SCRIPT] Executing action: $ACTION\"\n");
    fprintf(fp, "\n");
    fprintf(fp, "case \"$ACTION\" in\n");
    fprintf(fp, "    download)\n");
    fprintf(fp, "        echo \"[TEST-SCRIPT] Download phase - preparing files\"\n");
    fprintf(fp, "        exit 0\n");
    fprintf(fp, "        ;;\n");
    fprintf(fp, "    backup)\n");
    fprintf(fp, "        echo \"[TEST-SCRIPT] Backup phase - creating backup\"\n");
    fprintf(fp, "        exit 0\n");
    fprintf(fp, "        ;;\n");
    fprintf(fp, "    install)\n");
    fprintf(fp, "        echo \"[TEST-SCRIPT] Install phase - installing update\"\n");
    fprintf(fp, "        exit 0\n");
    fprintf(fp, "        ;;\n");
    fprintf(fp, "    apply)\n");
    fprintf(fp, "        echo \"[TEST-SCRIPT] Apply phase - applying changes\"\n");
    fprintf(fp, "        exit 0\n");
    fprintf(fp, "        ;;\n");
    fprintf(fp, "    restore)\n");
    fprintf(fp, "        echo \"[TEST-SCRIPT] Restore phase - rolling back\"\n");
    fprintf(fp, "        exit 0\n");
    fprintf(fp, "        ;;\n");
    fprintf(fp, "    cancel)\n");
    fprintf(fp, "        echo \"[TEST-SCRIPT] Cancel phase - cancelling operation\"\n");
    fprintf(fp, "        exit 0\n");
    fprintf(fp, "        ;;\n");
    fprintf(fp, "    isInstalled)\n");
    fprintf(fp, "        echo \"[TEST-SCRIPT] IsInstalled check - checking installation\"\n");
    fprintf(fp, "        exit 0\n");
    fprintf(fp, "        ;;\n");
    fprintf(fp, "    *)\n");
    fprintf(fp, "        echo \"[TEST-SCRIPT] Unknown action: $ACTION\"\n");
    fprintf(fp, "        exit 1\n");
    fprintf(fp, "        ;;\n");
    fprintf(fp, "esac\n");

    fclose(fp);

    // Make script executable
    if (chmod(scriptPath, 0755) != 0)
    {
        printf("Failed to make script executable\n");
        return -1;
    }

    return 0;
}

/**
 * @brief Test a single action
 */
static void TestAction(const ADUC_WorkflowData* workflowData, const char* action)
{
    ADUC_ResultDetails result;
    memset(&result, 0, sizeof(result));

    printf("\n=== Testing Action: %s ===\n", action);

    ADUC_Result_t actionResult = ScriptHandlerV3_ProcessAction(workflowData, action, &result);

    printf("Result: %s\n", (actionResult == ADUC_Result_Success) ? "SUCCESS" : "FAILED");
    if (result.resultDetails)
    {
        printf("Details: %s\n", result.resultDetails);
    }
    if (result.extendedResultCode != 0)
    {
        printf("Extended Code: %d\n", result.extendedResultCode);
    }
}

int main(void)
{
    printf("=== ADU Script Handler v3 Test ===\n");

    // Create test directory
    const char* testDir = "/tmp/adu-script-handler-test";
    const char* scriptPath = "/tmp/adu-script-handler-test/test-script.sh";

    if (mkdir(testDir, 0755) != 0 && errno != EEXIST)
    {
        printf("Failed to create test directory\n");
        return 1;
    }

    // Create test script
    if (CreateTestScript(scriptPath) != 0)
    {
        return 1;
    }

    printf("Created test script: %s\n", scriptPath);

    // Create mock workflow data
    ADUC_WorkflowData* workflowData = CreateMockWorkflowData(scriptPath);
    if (!workflowData)
    {
        printf("Failed to create mock workflow data\n");
        return 1;
    }

    // Test all actions
    const char* actions[] = {
        "download",
        "backup",
        "install",
        "apply",
        "restore",
        "cancel",
        "isInstalled"
    };

    for (size_t i = 0; i < sizeof(actions) / sizeof(actions[0]); i++)
    {
        TestAction(workflowData, actions[i]);
    }

    // Test invalid action
    TestAction(workflowData, "invalid");

    // Cleanup
    FreeMockWorkflowData(workflowData);
    unlink(scriptPath);
    rmdir(testDir);

    printf("\n=== Test Complete ===\n");
    return 0;
}
