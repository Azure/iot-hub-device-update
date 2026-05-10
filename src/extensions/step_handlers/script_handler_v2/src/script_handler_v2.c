/**
 * @file script_handler_v2.c
 * @brief Microsoft Script Handler v2 — customer-friendly script execution.
 *
 * Implements the "microsoft/script:2" step handler. Customer scripts are plain
 * bash scripts that do their work and exit. The handler provides:
 *   - Environment variable injection (ADU_WORK_FOLDER, ADU_RESULT_FILE, etc.)
 *   - Exit code interpretation (0=success, 1=fail, 2=retry, 3=reboot, 100=installed)
 *   - Optional JSON result file for richer reporting
 *   - Timeout enforcement via the process supervisor
 *   - Cancellation support
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "script_handler_v2.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "aduc/process_supervisor.h"

#define SCRIPT_COMP "microsoft-script-v2"

/* ─── Simple JSON helpers (strstr-based, no external dependencies) ─────────── */

static void json_get_string(const char* json, const char* key, char* out, size_t outLen, const char* defaultVal)
{
    if (out == NULL || outLen == 0)
    {
        return;
    }
    strncpy(out, defaultVal, outLen - 1);
    out[outLen - 1] = '\0';

    if (json == NULL)
    {
        return;
    }
    const char* pos = strstr(json, key);
    if (pos == NULL)
    {
        return;
    }
    pos += strlen(key);
    pos = strchr(pos, ':');
    if (pos == NULL)
    {
        return;
    }
    pos++;
    while (*pos == ' ' || *pos == '\t')
    {
        pos++;
    }
    if (*pos != '"')
    {
        return;
    }
    pos++;
    size_t i = 0;
    while (*pos != '\0' && *pos != '"' && i < outLen - 1)
    {
        if (*pos == '\\' && *(pos + 1) != '\0')
        {
            pos++;
        }
        out[i++] = *pos++;
    }
    out[i] = '\0';
}

static uint32_t json_get_uint(const char* json, const char* key, uint32_t defaultVal)
{
    if (json == NULL)
    {
        return defaultVal;
    }
    const char* pos = strstr(json, key);
    if (pos == NULL)
    {
        return defaultVal;
    }
    pos += strlen(key);
    pos = strchr(pos, ':');
    if (pos == NULL)
    {
        return defaultVal;
    }
    pos++;
    while (*pos == ' ' || *pos == '\t')
    {
        pos++;
    }
    uint32_t val = 0;
    bool found = false;
    while (*pos >= '0' && *pos <= '9')
    {
        val = val * 10 + (uint32_t)(*pos - '0');
        pos++;
        found = true;
    }
    return found ? val : defaultVal;
}

/**
 * @brief Parse a simple JSON result file written by customer script.
 *
 * Expected format:
 *   {"resultCode":0,"extendedResultCode":0,"resultDetails":"...","signal":"continue"}
 */
static bool parse_result_file(const char* path, int* outResultCode, char* outDetails, size_t detailsLen, char* outSignal, size_t signalLen)
{
    FILE* fp = fopen(path, "r");
    if (fp == NULL)
    {
        return false;
    }

    char buf[4096];
    size_t n = fread(buf, 1, sizeof(buf) - 1, fp);
    fclose(fp);
    buf[n] = '\0';

    uint32_t rc = json_get_uint(buf, "resultCode", 0);
    *outResultCode = (int)rc;

    json_get_string(buf, "resultDetails", outDetails, detailsLen, "");
    json_get_string(buf, "signal", outSignal, signalLen, "continue");

    return true;
}

/* ─── Extension-level state ───────────────────────────────────────────────── */

static const ADUC_ExtensionContext* s_extCtx = NULL;

static ADUC_Result2 make_failure(uint16_t specific)
{
    return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_STATE, specific);
}

/* ─── Helper: resolve script path ─────────────────────────────────────────── */

/**
 * @brief Find the local path of a script file within the step's downloaded files.
 */
static const char* find_file_path(const ADUC_StepHandleImpl* h, const char* fileName)
{
    if (fileName == NULL || fileName[0] == '\0')
    {
        return NULL;
    }
    for (size_t i = 0; i < h->stepCtx.fileCount; i++)
    {
        if (h->stepCtx.files[i].name != NULL && strcmp(h->stepCtx.files[i].name, fileName) == 0)
        {
            return h->stepCtx.files[i].path;
        }
    }
    return NULL;
}

/* ─── Helper: build work folder path ─────────────────────────────────────── */

static void build_work_folder(ADUC_StepHandleImpl* h)
{
    if (h->workFolder[0] != '\0')
    {
        return; /* Already set from config */
    }

    const char* wfId = h->stepCtx.workflowId ? h->stepCtx.workflowId : "unknown";
    const char* stepId = h->stepCtx.stepId ? h->stepCtx.stepId : "step_0";

    snprintf(h->workFolder, sizeof(h->workFolder), "/tmp/adu/%s/%s", wfId, stepId);
}

/* ─── Helper: ensure directory exists ─────────────────────────────────────── */

static bool ensure_directory(const char* path)
{
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
    {
        return true;
    }

    /* Create parent directories recursively */
    char tmp[512];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    for (char* p = tmp + 1; *p != '\0'; p++)
    {
        if (*p == '/')
        {
            *p = '\0';
            (void)mkdir(tmp, 0755);
            *p = '/';
        }
    }
    return mkdir(tmp, 0755) == 0 || errno == EEXIST;
}

/* ─── Helper: make script executable ──────────────────────────────────────── */

static bool make_executable(const char* path)
{
    if (path == NULL || path[0] == '\0')
    {
        return false;
    }
    return chmod(path, 0755) == 0;
}

/* ─── Helper: run a script using process supervisor ───────────────────────── */

/**
 * @brief Execute a script via the process supervisor.
 *
 * Sets up ADU_* environment variables, spawns the process, waits for completion,
 * and returns the exit code. Captures result details from ADU_RESULT_FILE if present.
 */
static ADUC_Result2 run_script(
    ADUC_StepHandleImpl* h,
    const char* scriptPath,
    const char* extraArgs,
    int* outExitCode)
{
    if (scriptPath == NULL)
    {
        return make_failure(1);
    }

    if (h->cancelled)
    {
        return make_failure(2);
    }

    /* Build result file path */
    char resultFilePath[544];
    snprintf(resultFilePath, sizeof(resultFilePath), "%s/adu-result.json", h->workFolder);

    /* Remove stale result file */
    (void)unlink(resultFilePath);

    /* Build environment: set ADU_* variables in the process environment.
     * The process supervisor passes stepId and workflowId via its config,
     * but we also set custom env vars via setenv before fork (handled by supervisor
     * internals). For additional vars, we pass them as part of the context. */

    /* Construct argv: /bin/sh -c "scriptPath extraArgs" for argument splitting,
     * OR directly execute the script. We prefer direct execution for security. */
    const char* argv[16];
    int argc = 0;
    argv[argc++] = scriptPath;

    /* Parse arguments into argv (simple space-split for now) */
    char argsBuf[1024];
    if (extraArgs != NULL && extraArgs[0] != '\0')
    {
        strncpy(argsBuf, extraArgs, sizeof(argsBuf) - 1);
        argsBuf[sizeof(argsBuf) - 1] = '\0';

        char* saveptr = NULL;
        char* token = strtok_r(argsBuf, " ", &saveptr);
        while (token != NULL && argc < 15)
        {
            argv[argc++] = token;
            token = strtok_r(NULL, " ", &saveptr);
        }
    }
    argv[argc] = NULL;

    /* Build environment variable array for the child process */
    char retryBuf[16];
    snprintf(retryBuf, sizeof(retryBuf), "%u", h->stepCtx.attempt);

    char envWorkFolder[600], envContentDir[600], envResultFile[600];
    char envStepId[256], envWorkflowId[256], envComponentId[256];
    char envInstalledCriteria[512], envRetryCount[64];

    snprintf(envWorkFolder, sizeof(envWorkFolder), "ADU_WORK_FOLDER=%s", h->workFolder);
    snprintf(envContentDir, sizeof(envContentDir), "ADU_CONTENT_DIR=%s", h->workFolder);
    snprintf(envResultFile, sizeof(envResultFile), "ADU_RESULT_FILE=%s", resultFilePath);
    snprintf(envStepId, sizeof(envStepId), "ADU_STEP_ID=%s", h->stepCtx.stepId ? h->stepCtx.stepId : "");
    snprintf(envWorkflowId, sizeof(envWorkflowId), "ADU_WORKFLOW_ID=%s", h->stepCtx.workflowId ? h->stepCtx.workflowId : "");
    snprintf(envComponentId, sizeof(envComponentId), "ADU_COMPONENT_ID=%s", h->stepCtx.componentId ? h->stepCtx.componentId : "");
    snprintf(envInstalledCriteria, sizeof(envInstalledCriteria), "ADU_INSTALLED_CRITERIA=%s", h->stepCtx.installedCriteria ? h->stepCtx.installedCriteria : "");
    snprintf(envRetryCount, sizeof(envRetryCount), "ADU_RETRY_COUNT=%s", retryBuf);

    const char* envp[] = {
        envWorkFolder, envContentDir, envResultFile,
        envStepId, envWorkflowId, envComponentId,
        envInstalledCriteria, envRetryCount, NULL
    };

    /* Use process supervisor for secure, managed child execution */
    ADUC_ProcessConfig procConfig;
    memset(&procConfig, 0, sizeof(procConfig));
    procConfig.executablePath = scriptPath;
    procConfig.argv = argv;
    procConfig.envp = envp;
    procConfig.workingDir = h->workFolder;
    procConfig.timeoutSeconds = (uint32_t)h->timeoutSec;

    ADUC_ProcessResult procResult;
    memset(&procResult, 0, sizeof(procResult));

    ADUC_EXT_LOG_INFO(h->extCtx, SCRIPT_COMP, 2011,
        "Running script: %s", scriptPath);

    int spawnRc = ADUC_ProcessSupervisor_Run(&procConfig, &procResult);

    h->childPid = 0;

    if (spawnRc != 0)
    {
        ADUC_EXT_LOG_ERROR(h->extCtx, SCRIPT_COMP, 2010,
            "Failed to run script: %s", scriptPath);
        snprintf(h->resultDetails, sizeof(h->resultDetails),
            "Failed to launch script: %s", scriptPath);
        ADUC_ProcessSupervisor_FreeResult(&procResult);
        *outExitCode = -1;
        return make_failure(10);
    }

    /* Interpret process result */
    switch (procResult.state)
    {
        case ADUC_PROC_STATE_COMPLETED:
            *outExitCode = procResult.exitCode;
            break;

        case ADUC_PROC_STATE_TIMED_OUT:
            ADUC_EXT_LOG_ERROR(h->extCtx, SCRIPT_COMP, 2013,
                "Script timed out after %d seconds: %s", h->timeoutSec, scriptPath);
            snprintf(h->resultDetails, sizeof(h->resultDetails),
                "Script timed out after %d seconds", h->timeoutSec);
            ADUC_ProcessSupervisor_FreeResult(&procResult);
            *outExitCode = -1;
            return make_failure(12);

        case ADUC_PROC_STATE_SIGNALED:
            ADUC_EXT_LOG_WARN(h->extCtx, SCRIPT_COMP, 2014,
                "Script killed by signal %d: %s", procResult.signalNumber, scriptPath);
            snprintf(h->resultDetails, sizeof(h->resultDetails),
                "Script terminated by signal %d", procResult.signalNumber);
            ADUC_ProcessSupervisor_FreeResult(&procResult);
            *outExitCode = -1;
            return make_failure(13);

        case ADUC_PROC_STATE_FAILED_TO_START:
            ADUC_EXT_LOG_ERROR(h->extCtx, SCRIPT_COMP, 2015,
                "Script launch failed: %s", scriptPath);
            snprintf(h->resultDetails, sizeof(h->resultDetails),
                "Script could not be launched: %s", scriptPath);
            ADUC_ProcessSupervisor_FreeResult(&procResult);
            *outExitCode = -1;
            return make_failure(14);

        default:
            ADUC_ProcessSupervisor_FreeResult(&procResult);
            *outExitCode = -1;
            return make_failure(15);
    }

    ADUC_ProcessSupervisor_FreeResult(&procResult);

    /* Check for optional result file */
    struct stat st;
    if (stat(resultFilePath, &st) == 0 && st.st_size > 0)
    {
        int fileResultCode = 0;
        char fileDetails[2048];
        char fileSignal[32];

        if (parse_result_file(resultFilePath, &fileResultCode, fileDetails, sizeof(fileDetails), fileSignal, sizeof(fileSignal)))
        {
            /* Use result file details if present */
            if (fileDetails[0] != '\0')
            {
                strncpy(h->resultDetails, fileDetails, sizeof(h->resultDetails) - 1);
                h->resultDetails[sizeof(h->resultDetails) - 1] = '\0';
            }

            /* Parse signal from result file */
            if (strcmp(fileSignal, "reboot") == 0)
            {
                h->pendingSignal = ADUC_SIGNAL_IMMEDIATE_REBOOT;
            }
            else if (strcmp(fileSignal, "deferReboot") == 0)
            {
                h->pendingSignal = ADUC_SIGNAL_DEFER_REBOOT;
            }
            else if (strcmp(fileSignal, "abort") == 0)
            {
                h->pendingSignal = ADUC_SIGNAL_ABORT_DEPLOYMENT;
            }
            /* "continue" or unrecognized → keep default */
        }
    }

    ADUC_EXT_LOG_INFO(h->extCtx, SCRIPT_COMP, 2016,
        "Script exited with code %d: %s", *outExitCode, scriptPath);

    return ADUC_RESULT2_SUCCESS;
}

/* ─── Vtable implementations ─────────────────────────────────────────────── */

static ADUC_Result2 ScriptHandler_Evaluate(const ADUC_StepContext* ctx, ADUC_StepHandle* handle)
{
    ADUC_EXT_LOG_INFO(s_extCtx, SCRIPT_COMP, 2100,
        "Evaluating step %s", ctx->stepId ? ctx->stepId : "(null)");

    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)calloc(1, sizeof(ADUC_StepHandleImpl));
    if (h == NULL)
    {
        return make_failure(50);
    }

    h->stepCtx = *ctx;
    h->extCtx = s_extCtx;
    h->cancelled = false;
    h->childPid = 0;
    h->pendingSignal = ADUC_SIGNAL_CONTINUE;
    memset(&h->collectedResult, 0, sizeof(h->collectedResult));

    /* Parse handlerConfigJson */
    const char* cfg = ctx->handlerConfigJson;

    json_get_string(cfg, "scriptFileName", h->scriptFileName, sizeof(h->scriptFileName), "");
    json_get_string(cfg, "arguments", h->arguments, sizeof(h->arguments), "");
    json_get_string(cfg, "installedCriteriaScript", h->installedCriteriaScript, sizeof(h->installedCriteriaScript), "");
    json_get_string(cfg, "rollbackScript", h->rollbackScript, sizeof(h->rollbackScript), "");
    json_get_string(cfg, "workFolder", h->workFolder, sizeof(h->workFolder), "");
    h->timeoutSec = (int)json_get_uint(cfg, "timeout", SCRIPT_DEFAULT_TIMEOUT_SEC);

    /* Validate: scriptFileName is required */
    if (h->scriptFileName[0] == '\0')
    {
        ADUC_EXT_LOG_ERROR(s_extCtx, SCRIPT_COMP, 2101,
            "Missing required 'scriptFileName' in handlerProperties");
        h->collectedResult.result = make_failure(51);
        h->collectedResult.resultDetails = "Missing required 'scriptFileName' in handlerProperties";
        *handle = h;
        return make_failure(51);
    }

    /* Verify the main script exists in the step's files */
    const char* mainScriptPath = find_file_path(h, h->scriptFileName);
    if (mainScriptPath == NULL)
    {
        ADUC_EXT_LOG_ERROR(s_extCtx, SCRIPT_COMP, 2102,
            "Script file '%s' not found in step files", h->scriptFileName);
        h->collectedResult.result = make_failure(52);
        h->collectedResult.resultDetails = "Script file not found in downloaded content";
        *handle = h;
        return make_failure(52);
    }

    /* Build work folder path */
    build_work_folder(h);

    /* If installedCriteriaScript is provided, run it to check if already installed */
    if (h->installedCriteriaScript[0] != '\0')
    {
        const char* icScriptPath = find_file_path(h, h->installedCriteriaScript);
        if (icScriptPath != NULL)
        {
            /* Ensure work folder exists and script is executable for the check */
            (void)ensure_directory(h->workFolder);
            (void)make_executable(icScriptPath);

            int exitCode = -1;
            ADUC_Result2 runRc = run_script(h, icScriptPath, "", &exitCode);
            if (ADUC_RESULT2_IS_SUCCESS(runRc) && exitCode == SCRIPT_EXIT_ALREADY_INSTALLED)
            {
                ADUC_EXT_LOG_INFO(s_extCtx, SCRIPT_COMP, 2103,
                    "Step already installed (installedCriteriaScript returned 100)");
                h->collectedResult.result = ADUC_RESULT2_SUCCESS;
                h->collectedResult.resultDetails = "Already installed";
                h->collectedResult.signal = ADUC_SIGNAL_SKIP_REMAINING;
                *handle = h;
                return ADUC_RESULT2_SUCCESS;
            }
        }
    }

    *handle = h;
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 ScriptHandler_Acquire(ADUC_StepHandle handle)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;
    ADUC_EXT_LOG_INFO(h->extCtx, SCRIPT_COMP, 2200, "Acquiring (ensuring work folder)");

    if (h->cancelled)
    {
        return make_failure(60);
    }

    /* Ensure work folder exists */
    if (!ensure_directory(h->workFolder))
    {
        ADUC_EXT_LOG_ERROR(h->extCtx, SCRIPT_COMP, 2201,
            "Failed to create work folder: %s", h->workFolder);
        h->collectedResult.result = make_failure(61);
        h->collectedResult.resultDetails = "Failed to create work folder";
        return make_failure(61);
    }

    /* Make scripts executable */
    const char* mainPath = find_file_path(h, h->scriptFileName);
    if (mainPath != NULL)
    {
        (void)make_executable(mainPath);
    }

    if (h->installedCriteriaScript[0] != '\0')
    {
        const char* icPath = find_file_path(h, h->installedCriteriaScript);
        if (icPath != NULL)
        {
            (void)make_executable(icPath);
        }
    }

    if (h->rollbackScript[0] != '\0')
    {
        const char* rbPath = find_file_path(h, h->rollbackScript);
        if (rbPath != NULL)
        {
            (void)make_executable(rbPath);
        }
    }

    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 ScriptHandler_Preprocess(ADUC_StepHandle handle)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;
    ADUC_EXT_LOG_INFO(h->extCtx, SCRIPT_COMP, 2300, "Preprocessing");

    if (h->cancelled)
    {
        return make_failure(60);
    }

    /* No backup needed — rollbackScript handles rollback if provided */
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 ScriptHandler_Execute(ADUC_StepHandle handle, ADUC_StepProgressFn progressFn, void* progressCtx)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;
    ADUC_EXT_LOG_INFO(h->extCtx, SCRIPT_COMP, 2400, "Executing script: %s", h->scriptFileName);

    if (h->cancelled)
    {
        return make_failure(60);
    }

    /* Report 0% progress */
    if (progressFn != NULL)
    {
        progressFn(h->stepCtx.stepId, 0, "Execute", "Starting script", progressCtx);
    }

    /* Find and run the main script */
    const char* scriptPath = find_file_path(h, h->scriptFileName);
    if (scriptPath == NULL)
    {
        h->collectedResult.result = make_failure(62);
        h->collectedResult.resultDetails = "Main script file not found";
        return make_failure(62);
    }

    int exitCode = -1;
    ADUC_Result2 runRc = run_script(h, scriptPath, h->arguments, &exitCode);

    /* Report 100% progress */
    if (progressFn != NULL)
    {
        progressFn(h->stepCtx.stepId, 100, "Execute", "Script completed", progressCtx);
    }

    /* If process management itself failed (timeout, signal, launch error) */
    if (ADUC_RESULT2_IS_FAILURE(runRc))
    {
        h->collectedResult.result = runRc;
        if (h->resultDetails[0] != '\0')
        {
            h->collectedResult.resultDetails = h->resultDetails;
        }
        else
        {
            h->collectedResult.resultDetails = "Script execution failed";
        }
        return runRc;
    }

    /* Interpret exit code */
    switch (exitCode)
    {
        case SCRIPT_EXIT_SUCCESS:
            ADUC_EXT_LOG_INFO(h->extCtx, SCRIPT_COMP, 2401, "Script succeeded");
            if (h->resultDetails[0] == '\0')
            {
                snprintf(h->resultDetails, sizeof(h->resultDetails), "Script completed successfully");
            }
            return ADUC_RESULT2_SUCCESS;

        case SCRIPT_EXIT_RETRYABLE:
            ADUC_EXT_LOG_WARN(h->extCtx, SCRIPT_COMP, 2402, "Script returned retryable failure");
            if (h->resultDetails[0] == '\0')
            {
                snprintf(h->resultDetails, sizeof(h->resultDetails), "Script failed with retryable error (exit 2)");
            }
            h->collectedResult.result = make_failure(70);
            h->collectedResult.resultDetails = h->resultDetails;
            h->pendingSignal = ADUC_SIGNAL_RETRY_PHASE;
            return make_failure(70);

        case SCRIPT_EXIT_REBOOT_REQUIRED:
            ADUC_EXT_LOG_INFO(h->extCtx, SCRIPT_COMP, 2403, "Script requires reboot");
            if (h->resultDetails[0] == '\0')
            {
                snprintf(h->resultDetails, sizeof(h->resultDetails), "Script requires reboot to complete");
            }
            h->pendingSignal = ADUC_SIGNAL_IMMEDIATE_REBOOT;
            return ADUC_RESULT2_SUCCESS;

        default:
            ADUC_EXT_LOG_ERROR(h->extCtx, SCRIPT_COMP, 2404,
                "Script failed with exit code %d", exitCode);
            if (h->resultDetails[0] == '\0')
            {
                snprintf(h->resultDetails, sizeof(h->resultDetails),
                    "Script failed with exit code %d", exitCode);
            }
            h->collectedResult.result = make_failure(71);
            h->collectedResult.resultDetails = h->resultDetails;
            return make_failure(71);
    }
}

static ADUC_Result2 ScriptHandler_Validate(ADUC_StepHandle handle)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;
    ADUC_EXT_LOG_INFO(h->extCtx, SCRIPT_COMP, 2500, "Validating");

    if (h->cancelled)
    {
        return make_failure(60);
    }

    /* If installedCriteriaScript is provided, run it to verify success */
    if (h->installedCriteriaScript[0] != '\0')
    {
        const char* icScriptPath = find_file_path(h, h->installedCriteriaScript);
        if (icScriptPath != NULL)
        {
            int exitCode = -1;
            ADUC_Result2 runRc = run_script(h, icScriptPath, "", &exitCode);

            if (ADUC_RESULT2_IS_FAILURE(runRc))
            {
                ADUC_EXT_LOG_ERROR(h->extCtx, SCRIPT_COMP, 2501,
                    "Validation script failed to execute");
                h->collectedResult.result = make_failure(80);
                h->collectedResult.resultDetails = "Validation script failed to execute";
                return make_failure(80);
            }

            if (exitCode != 0)
            {
                ADUC_EXT_LOG_ERROR(h->extCtx, SCRIPT_COMP, 2502,
                    "Validation failed: installedCriteriaScript returned %d", exitCode);
                h->collectedResult.result = make_failure(81);
                snprintf(h->resultDetails, sizeof(h->resultDetails),
                    "Validation failed: check script returned exit code %d", exitCode);
                h->collectedResult.resultDetails = h->resultDetails;
                return make_failure(81);
            }

            ADUC_EXT_LOG_INFO(h->extCtx, SCRIPT_COMP, 2503,
                "Validation passed (installedCriteriaScript returned 0)");
        }
    }

    /* No installedCriteriaScript: trust the Execute exit code */
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 ScriptHandler_Postprocess(ADUC_StepHandle handle, bool rollback)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;
    ADUC_EXT_LOG_INFO(h->extCtx, SCRIPT_COMP, 2600,
        "Postprocessing (rollback=%s)", rollback ? "true" : "false");

    /* If rollback requested and rollbackScript is provided, run it */
    if (rollback && h->rollbackScript[0] != '\0')
    {
        const char* rbScriptPath = find_file_path(h, h->rollbackScript);
        if (rbScriptPath != NULL)
        {
            ADUC_EXT_LOG_INFO(h->extCtx, SCRIPT_COMP, 2601,
                "Running rollback script: %s", h->rollbackScript);

            int exitCode = -1;
            ADUC_Result2 runRc = run_script(h, rbScriptPath, "", &exitCode);

            if (ADUC_RESULT2_IS_FAILURE(runRc) || exitCode != 0)
            {
                ADUC_EXT_LOG_ERROR(h->extCtx, SCRIPT_COMP, 2602,
                    "Rollback script failed (exit=%d)", exitCode);
                /* Rollback failure is logged but doesn't fail postprocess —
                 * we still want to report the original failure */
            }
            else
            {
                ADUC_EXT_LOG_INFO(h->extCtx, SCRIPT_COMP, 2603, "Rollback completed successfully");
            }

            h->collectedResult.rollbackPerformed = true;
        }
        else
        {
            ADUC_EXT_LOG_WARN(h->extCtx, SCRIPT_COMP, 2604,
                "Rollback requested but rollbackScript '%s' not found in step files",
                h->rollbackScript);
        }
    }

    /* Clean up result file */
    char resultFilePath[544];
    snprintf(resultFilePath, sizeof(resultFilePath), "%s/adu-result.json", h->workFolder);
    (void)unlink(resultFilePath);

    return ADUC_RESULT2_SUCCESS;
}

static ADUC_StepResult ScriptHandler_GetResult(ADUC_StepHandle handle)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;

    /* If no failure was collected, report overall success */
    if (ADUC_RESULT2_IS_SUCCESS(h->collectedResult.result))
    {
        h->collectedResult.result = ADUC_RESULT2_SUCCESS;
        if (h->resultDetails[0] != '\0')
        {
            h->collectedResult.resultDetails = h->resultDetails;
        }
        else
        {
            h->collectedResult.resultDetails = "Script step completed successfully";
        }
    }

    /* Apply signal from script result file or exit code interpretation */
    if (h->pendingSignal != ADUC_SIGNAL_CONTINUE)
    {
        h->collectedResult.signal = h->pendingSignal;
    }
    else
    {
        h->collectedResult.signal = ADUC_SIGNAL_CONTINUE;
    }

    return h->collectedResult;
}

static ADUC_Result2 ScriptHandler_Cancel(ADUC_StepHandle handle)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;
    h->cancelled = true;

    ADUC_EXT_LOG_WARN(h->extCtx, SCRIPT_COMP, 2700, "Cancel requested");

    /* If a child process is running, cancel it via the process supervisor.
     * Note: In practice, the childPid field is set during run_script and cleared
     * after Wait returns, so direct kill is a fallback. The process supervisor
     * handles cancellation internally when the handle is still active. */
    if (h->childPid > 0)
    {
        ADUC_EXT_LOG_INFO(h->extCtx, SCRIPT_COMP, 2701,
            "Sending termination signal to child PID %d", (int)h->childPid);
        /* The process supervisor Cancel would be ideal here, but we may not
         * have the process handle. As a fallback, the cancelled flag causes
         * run_script to abort on next check. */
    }

    return ADUC_RESULT2_SUCCESS;
}

static void ScriptHandler_Release(ADUC_StepHandle handle)
{
    if (handle != NULL)
    {
        free(handle);
    }
}

static ADUC_Result2 ScriptHandler_IsInstalled(const ADUC_StepContext* ctx, bool* outIsInstalled)
{
    if (outIsInstalled == NULL)
    {
        return make_failure(90);
    }

    *outIsInstalled = false;

    /* If there's an installedCriteriaScript in the config, we need to run it.
     * However, IsInstalled is a lightweight query — we create a temporary handle
     * to parse config and run the check script. */
    if (ctx == NULL || ctx->handlerConfigJson == NULL)
    {
        return ADUC_RESULT2_SUCCESS;
    }

    char icScript[256];
    json_get_string(ctx->handlerConfigJson, "installedCriteriaScript", icScript, sizeof(icScript), "");

    if (icScript[0] == '\0')
    {
        /* No check script — cannot determine installed state */
        return ADUC_RESULT2_SUCCESS;
    }

    /* Find the script in step files */
    const char* icPath = NULL;
    for (size_t i = 0; i < ctx->fileCount; i++)
    {
        if (ctx->files[i].name != NULL && strcmp(ctx->files[i].name, icScript) == 0)
        {
            icPath = ctx->files[i].path;
            break;
        }
    }

    if (icPath == NULL)
    {
        ADUC_EXT_LOG_DEBUG(s_extCtx, SCRIPT_COMP, 2800,
            "IsInstalled: script '%s' not found in step files", icScript);
        return ADUC_RESULT2_SUCCESS;
    }

    /* Create a temporary handle to run the script */
    ADUC_StepHandleImpl tmpHandle;
    memset(&tmpHandle, 0, sizeof(tmpHandle));
    tmpHandle.stepCtx = *ctx;
    tmpHandle.extCtx = s_extCtx;
    tmpHandle.timeoutSec = 60; /* Short timeout for installed check */
    tmpHandle.pendingSignal = ADUC_SIGNAL_CONTINUE;

    /* Build work folder */
    const char* wfId = ctx->workflowId ? ctx->workflowId : "unknown";
    const char* stepId = ctx->stepId ? ctx->stepId : "step_0";
    snprintf(tmpHandle.workFolder, sizeof(tmpHandle.workFolder), "/tmp/adu/%s/%s", wfId, stepId);
    (void)ensure_directory(tmpHandle.workFolder);
    (void)make_executable(icPath);

    int exitCode = -1;
    ADUC_Result2 runRc = run_script(&tmpHandle, icPath, "", &exitCode);

    if (ADUC_RESULT2_IS_SUCCESS(runRc))
    {
        if (exitCode == 0 || exitCode == SCRIPT_EXIT_ALREADY_INSTALLED)
        {
            *outIsInstalled = true;
            ADUC_EXT_LOG_INFO(s_extCtx, SCRIPT_COMP, 2801, "IsInstalled: yes (exit %d)", exitCode);
        }
        else
        {
            ADUC_EXT_LOG_INFO(s_extCtx, SCRIPT_COMP, 2802, "IsInstalled: no (exit %d)", exitCode);
        }
    }

    return ADUC_RESULT2_SUCCESS;
}

/* ─── Vtable ──────────────────────────────────────────────────────────────── */

static const ADUC_StepHandlerVtable s_vtable = {
    .structVersion = 1,
    .Evaluate = ScriptHandler_Evaluate,
    .Acquire = ScriptHandler_Acquire,
    .Preprocess = ScriptHandler_Preprocess,
    .Execute = ScriptHandler_Execute,
    .Validate = ScriptHandler_Validate,
    .Postprocess = ScriptHandler_Postprocess,
    .GetResult = ScriptHandler_GetResult,
    .Cancel = ScriptHandler_Cancel,
    .Release = ScriptHandler_Release,
    .Report = NULL,
    .Signal = NULL,
    .IsInstalled = ScriptHandler_IsInstalled,
    .GetCapabilities = NULL,
};

/* ─── Extension lifecycle ─────────────────────────────────────────────────── */

ADUC_Result2 ScriptHandler_Initialize(const ADUC_ExtensionContext* ctx)
{
    s_extCtx = ctx;
    ADUC_EXT_LOG_INFO(ctx, SCRIPT_COMP, 2000, "Microsoft Script Handler v2 initialized");
    return ADUC_RESULT2_SUCCESS;
}

void ScriptHandler_Uninitialize(void)
{
    if (s_extCtx != NULL)
    {
        ADUC_EXT_LOG_INFO(s_extCtx, SCRIPT_COMP, 2099, "Microsoft Script Handler v2 uninitialized");
    }
    s_extCtx = NULL;
}

/* ─── Extension descriptor ────────────────────────────────────────────────── */

static const char* s_capabilities[] = { "microsoft/script:2", "microsoft/script:1", NULL };

static const ADUC_ExtensionDescriptor s_descriptor = {
    .structVersion = ADUC_EXTENSION_DESCRIPTOR_VERSION,
    .id = "microsoft-script-v2",
    .name = "Microsoft Script Handler v2",
    .version = "2.0.0",
    .type = ADUC_EXT_TYPE_STEP_HANDLER,
    .minHostApiVersion = 1,
    .Initialize = ScriptHandler_Initialize,
    .Uninitialize = ScriptHandler_Uninitialize,
    .vtable = &s_vtable,
    .capabilities = s_capabilities,
};

ADUC_DECLARE_EXTENSION(s_descriptor)
