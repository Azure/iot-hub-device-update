/**
 * @file swupdate_handler_v2.c
 * @brief Microsoft SWUpdate Handler v2 — installs firmware images via SWUpdate
 *        with progress reporting and optional verification.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "swupdate_handler_v2.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <parson.h>

#define SWU_COMP "microsoft-swupdate-v2"

/* ─── Extension-level state ───────────────────────────────────────────────── */

static const ADUC_ExtensionContext* s_extCtx = NULL;

/* ─── Error helpers ───────────────────────────────────────────────────────── */

static ADUC_Result2 make_failure(uint16_t specific)
{
    return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_IO, specific);
}

static ADUC_Result2 make_config_error(uint16_t specific)
{
    return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_CONFIG, specific);
}

/* ─── Process execution helper ────────────────────────────────────────────── */

static int run_command(
    const char* path,
    char* const argv[],
    char* const envp[],
    char* outputBuf,
    size_t outputBufSize,
    int timeoutSec)
{
    int pipeFds[2];
    if (pipe(pipeFds) != 0)
    {
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        close(pipeFds[0]);
        close(pipeFds[1]);
        return -1;
    }

    if (pid == 0)
    {
        /* Child process */
        close(pipeFds[0]);
        dup2(pipeFds[1], STDOUT_FILENO);
        dup2(pipeFds[1], STDERR_FILENO);
        close(pipeFds[1]);

        if (envp != NULL)
        {
            execve(path, argv, envp);
        }
        else
        {
            execv(path, argv);
        }
        _exit(127);
    }

    /* Parent process */
    close(pipeFds[1]);

    size_t totalRead = 0;
    if (outputBuf != NULL && outputBufSize > 0)
    {
        outputBuf[0] = '\0';

        int flags = fcntl(pipeFds[0], F_GETFL, 0);
        (void)fcntl(pipeFds[0], F_SETFL, flags | O_NONBLOCK);

        int elapsed = 0;
        while (elapsed < timeoutSec)
        {
            ssize_t n = read(pipeFds[0], outputBuf + totalRead, outputBufSize - totalRead - 1);
            if (n > 0)
            {
                totalRead += (size_t)n;
                if (totalRead >= outputBufSize - 1)
                {
                    break;
                }
            }
            else if (n == 0)
            {
                break; /* EOF */
            }
            else
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    int status = 0;
                    pid_t w = waitpid(pid, &status, WNOHANG);
                    if (w == pid)
                    {
                        ssize_t r = read(pipeFds[0], outputBuf + totalRead, outputBufSize - totalRead - 1);
                        if (r > 0)
                        {
                            totalRead += (size_t)r;
                        }
                        outputBuf[totalRead] = '\0';
                        close(pipeFds[0]);
                        if (WIFEXITED(status))
                        {
                            return WEXITSTATUS(status);
                        }
                        return -1;
                    }
                    usleep(100000); /* 100ms */
                    elapsed++;
                }
                else
                {
                    break;
                }
            }
        }
        outputBuf[totalRead] = '\0';
    }

    close(pipeFds[0]);

    int status = 0;
    pid_t w = waitpid(pid, &status, WNOHANG);
    if (w == 0)
    {
        int waitAttempts = 0;
        while (waitAttempts < timeoutSec * 10)
        {
            w = waitpid(pid, &status, WNOHANG);
            if (w != 0)
            {
                break;
            }
            usleep(100000);
            waitAttempts++;
        }
        if (w == 0)
        {
            kill(pid, SIGTERM);
            usleep(500000);
            waitpid(pid, &status, WNOHANG);
            return -2; /* Timeout */
        }
    }

    if (WIFEXITED(status))
    {
        return WEXITSTATUS(status);
    }
    return -1;
}

/* ─── JSON config parsing ─────────────────────────────────────────────────── */

static bool parse_config(const char* json, SwuHandlerConfig* cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->rebootRequired = true; /* Default */

    if (json == NULL || json[0] == '\0')
    {
        return false;
    }

    JSON_Value* rootVal = json_parse_string(json);
    if (rootVal == NULL)
    {
        return false;
    }

    JSON_Object* root = json_value_get_object(rootVal);
    if (root == NULL)
    {
        json_value_free(rootVal);
        return false;
    }

    /* swuFileName (required) */
    const char* swuFile = json_object_get_string(root, "swuFileName");
    if (swuFile == NULL || swuFile[0] == '\0')
    {
        json_value_free(rootVal);
        return false;
    }
    strncpy(cfg->swuFileName, swuFile, SWU_MAX_PATH_LEN - 1);
    cfg->swuFileName[SWU_MAX_PATH_LEN - 1] = '\0';

    /* hwRevision (optional) */
    const char* hwRev = json_object_get_string(root, "hwRevision");
    if (hwRev != NULL)
    {
        strncpy(cfg->hwRevision, hwRev, SWU_MAX_HWREV_LEN - 1);
        cfg->hwRevision[SWU_MAX_HWREV_LEN - 1] = '\0';
    }

    /* rebootRequired (optional, default true) */
    if (json_object_has_value_of_type(root, "rebootRequired", JSONBoolean))
    {
        cfg->rebootRequired = (bool)json_object_get_boolean(root, "rebootRequired");
    }

    /* verifyCommand (optional) */
    const char* verCmd = json_object_get_string(root, "verifyCommand");
    if (verCmd != NULL)
    {
        strncpy(cfg->verifyCommand, verCmd, SWU_MAX_PATH_LEN - 1);
        cfg->verifyCommand[SWU_MAX_PATH_LEN - 1] = '\0';
    }

    /* expectedVersion (optional) */
    const char* expVer = json_object_get_string(root, "expectedVersion");
    if (expVer != NULL)
    {
        strncpy(cfg->expectedVersion, expVer, SWU_MAX_VERSION_LEN - 1);
        cfg->expectedVersion[SWU_MAX_VERSION_LEN - 1] = '\0';
    }

    json_value_free(rootVal);
    return true;
}

/* ─── Verification helper ─────────────────────────────────────────────────── */

static bool run_verify_command(const ADUC_StepHandleImpl* h, char* outputBuf, size_t outputBufSize)
{
    if (h->config.verifyCommand[0] == '\0')
    {
        return false;
    }

    /* Split verifyCommand into path and args */
    char cmdCopy[SWU_MAX_PATH_LEN];
    strncpy(cmdCopy, h->config.verifyCommand, sizeof(cmdCopy) - 1);
    cmdCopy[sizeof(cmdCopy) - 1] = '\0';

    char* argv[32];
    int argc = 0;
    char* saveptr = NULL;
    char* tok = strtok_r(cmdCopy, " ", &saveptr);
    while (tok != NULL && argc < 31)
    {
        argv[argc++] = tok;
        tok = strtok_r(NULL, " ", &saveptr);
    }
    argv[argc] = NULL;

    if (argc == 0)
    {
        return false;
    }

    int rc = run_command(argv[0], argv, NULL, outputBuf, outputBufSize, 30);
    if (rc != 0)
    {
        return false;
    }

    /* Trim trailing whitespace */
    size_t len = strlen(outputBuf);
    while (len > 0 && (outputBuf[len - 1] == '\n' || outputBuf[len - 1] == '\r' || outputBuf[len - 1] == ' '))
    {
        outputBuf[--len] = '\0';
    }

    return true;
}

/* ─── Progress parsing from swupdate output ───────────────────────────────── */

static uint32_t parse_swupdate_progress(const char* output)
{
    /*
     * SWUpdate verbose output contains lines like:
     *   [TRACE] : SWUPDATE running :  <percent>% ...
     * Scan backwards for the last percentage reported.
     */
    uint32_t lastPercent = 0;
    const char* pos = output;
    while ((pos = strstr(pos, "%")) != NULL)
    {
        /* Walk backwards to find the number before % */
        const char* numEnd = pos;
        const char* numStart = pos - 1;
        while (numStart >= output && *numStart >= '0' && *numStart <= '9')
        {
            numStart--;
        }
        numStart++;
        if (numStart < numEnd)
        {
            char numBuf[8];
            size_t numLen = (size_t)(numEnd - numStart);
            if (numLen < sizeof(numBuf))
            {
                memcpy(numBuf, numStart, numLen);
                numBuf[numLen] = '\0';
                uint32_t pct = (uint32_t)atoi(numBuf);
                if (pct <= 100 && pct > lastPercent)
                {
                    lastPercent = pct;
                }
            }
        }
        pos++;
    }
    return lastPercent;
}

/* ─── Vtable implementations ─────────────────────────────────────────────── */

static ADUC_Result2 SwuHandler_Evaluate(const ADUC_StepContext* ctx, ADUC_StepHandle* handle)
{
    ADUC_EXT_LOG_INFO(s_extCtx, SWU_COMP, 3001, "Evaluating SWUpdate step %s", ctx->stepId ? ctx->stepId : "(null)");

    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)calloc(1, sizeof(ADUC_StepHandleImpl));
    if (h == NULL)
    {
        return make_failure(1);
    }

    h->stepCtx = *ctx;
    h->extCtx = s_extCtx;
    h->cancelled = false;
    memset(&h->collectedResult, 0, sizeof(h->collectedResult));

    if (!parse_config(ctx->handlerConfigJson, &h->config))
    {
        ADUC_EXT_LOG_ERROR(s_extCtx, SWU_COMP, 3002, "Failed to parse handlerProperties");
        snprintf(h->resultDetails, sizeof(h->resultDetails), "Invalid handlerProperties JSON or missing swuFileName");
        h->collectedResult.result = make_config_error(1);
        h->collectedResult.resultDetails = h->resultDetails;
        *handle = h;
        return make_config_error(1);
    }

    /* Find swuFileName in step files */
    bool found = false;
    for (size_t i = 0; i < ctx->fileCount; i++)
    {
        if (ctx->files[i].name != NULL && strcmp(ctx->files[i].name, h->config.swuFileName) == 0)
        {
            if (ctx->files[i].path != NULL)
            {
                strncpy(h->swuFilePath, ctx->files[i].path, SWU_MAX_PATH_LEN - 1);
                h->swuFilePath[SWU_MAX_PATH_LEN - 1] = '\0';
                h->swuFileSize = ctx->files[i].size;
                found = true;
            }
            break;
        }
    }

    if (!found)
    {
        ADUC_EXT_LOG_ERROR(s_extCtx, SWU_COMP, 3003, "SWU file '%s' not found in step files", h->config.swuFileName);
        snprintf(h->resultDetails, sizeof(h->resultDetails),
                 "SWU file '%s' not found in step files", h->config.swuFileName);
        h->collectedResult.result = make_config_error(2);
        h->collectedResult.resultDetails = h->resultDetails;
        *handle = h;
        return make_config_error(2);
    }

    /* Verify file exists on disk */
    struct stat st;
    if (stat(h->swuFilePath, &st) != 0)
    {
        ADUC_EXT_LOG_ERROR(s_extCtx, SWU_COMP, 3004, "SWU file not on disk: %s", h->swuFilePath);
        snprintf(h->resultDetails, sizeof(h->resultDetails),
                 "SWU file does not exist on disk: %s", h->swuFilePath);
        h->collectedResult.result = make_failure(2);
        h->collectedResult.resultDetails = h->resultDetails;
        *handle = h;
        return make_failure(2);
    }

    ADUC_EXT_LOG_INFO(s_extCtx, SWU_COMP, 3005, "SWU file: %s (size=%llu)",
                      h->swuFilePath, (unsigned long long)h->swuFileSize);

    *handle = h;
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 SwuHandler_Acquire(ADUC_StepHandle handle)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;

    if (h->cancelled)
    {
        return make_failure(10);
    }

    ADUC_EXT_LOG_INFO(h->extCtx, SWU_COMP, 3010, "Acquire: verifying SWU file integrity");

    /* Verify file size matches manifest */
    struct stat st;
    if (stat(h->swuFilePath, &st) != 0)
    {
        ADUC_EXT_LOG_ERROR(h->extCtx, SWU_COMP, 3011, "Cannot stat SWU file: %s", h->swuFilePath);
        snprintf(h->resultDetails, sizeof(h->resultDetails), "Cannot stat SWU file: %s", h->swuFilePath);
        h->collectedResult.result = make_failure(11);
        h->collectedResult.resultDetails = h->resultDetails;
        return make_failure(11);
    }

    if (h->swuFileSize > 0 && (uint64_t)st.st_size != h->swuFileSize)
    {
        ADUC_EXT_LOG_ERROR(h->extCtx, SWU_COMP, 3012,
                           "SWU file size mismatch: expected=%llu actual=%llu",
                           (unsigned long long)h->swuFileSize, (unsigned long long)st.st_size);
        snprintf(h->resultDetails, sizeof(h->resultDetails),
                 "SWU file size mismatch: expected %llu, got %llu",
                 (unsigned long long)h->swuFileSize, (unsigned long long)st.st_size);
        h->collectedResult.result = make_failure(12);
        h->collectedResult.resultDetails = h->resultDetails;
        return make_failure(12);
    }

    ADUC_EXT_LOG_INFO(h->extCtx, SWU_COMP, 3013, "SWU file integrity check passed");
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 SwuHandler_Preprocess(ADUC_StepHandle handle)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;

    if (h->cancelled)
    {
        return make_failure(20);
    }

    ADUC_EXT_LOG_INFO(h->extCtx, SWU_COMP, 3020, "Preprocess: checking SWUpdate availability");

    /* Check if swupdate binary is available */
    char output[256];
    char* argv[] = { "/usr/bin/which", "swupdate", NULL };
    int rc = run_command("/usr/bin/which", argv, NULL, output, sizeof(output), 10);
    if (rc != 0)
    {
        ADUC_EXT_LOG_ERROR(h->extCtx, SWU_COMP, 3021, "SWUpdate binary not found on system");
        snprintf(h->resultDetails, sizeof(h->resultDetails), "SWUpdate binary not found (which swupdate failed)");
        h->collectedResult.result = make_failure(21);
        h->collectedResult.resultDetails = h->resultDetails;
        return make_failure(21);
    }

    ADUC_EXT_LOG_INFO(h->extCtx, SWU_COMP, 3022, "SWUpdate found: %.100s", output);
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 SwuHandler_Execute(ADUC_StepHandle handle, ADUC_StepProgressFn progressFn, void* progressCtx)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;

    if (h->cancelled)
    {
        return make_failure(30);
    }

    ADUC_EXT_LOG_INFO(h->extCtx, SWU_COMP, 3030, "Execute: running swupdate -i %s", h->swuFilePath);

    if (progressFn != NULL)
    {
        progressFn(h->stepCtx.stepId, 0, "Execute", "Starting SWUpdate", progressCtx);
    }

    /* Build argv: swupdate -i <file> [-H <hwrev>] -v */
    char* argv[16];
    int argc = 0;

    argv[argc++] = "/usr/bin/swupdate";
    argv[argc++] = "-i";
    argv[argc++] = h->swuFilePath;

    if (h->config.hwRevision[0] != '\0')
    {
        argv[argc++] = "-H";
        argv[argc++] = h->config.hwRevision;
    }

    argv[argc++] = "-v";
    argv[argc] = NULL;

    char output[SWU_OUTPUT_BUF_SIZE];
    int rc = run_command("/usr/bin/swupdate", argv, NULL, output, sizeof(output), SWU_DEFAULT_TIMEOUT_SEC);

    if (h->cancelled)
    {
        return make_failure(31);
    }

    /* Parse progress from output */
    uint32_t progressPct = parse_swupdate_progress(output);
    if (progressFn != NULL && progressPct > 0)
    {
        progressFn(h->stepCtx.stepId, progressPct, "Execute", "SWUpdate progress", progressCtx);
    }

    if (rc == -2)
    {
        ADUC_EXT_LOG_ERROR(h->extCtx, SWU_COMP, 3031, "SWUpdate timed out after %ds", SWU_DEFAULT_TIMEOUT_SEC);
        snprintf(h->resultDetails, sizeof(h->resultDetails), "SWUpdate timed out after %ds", SWU_DEFAULT_TIMEOUT_SEC);
        h->collectedResult.result = make_failure(32);
        h->collectedResult.resultDetails = h->resultDetails;
        return make_failure(32);
    }

    if (rc != 0)
    {
        ADUC_EXT_LOG_ERROR(h->extCtx, SWU_COMP, 3032, "SWUpdate failed (rc=%d): %.200s", rc, output);
        snprintf(h->resultDetails, sizeof(h->resultDetails),
                 "SWUpdate failed (exit code %d): %.1800s", rc, output);
        h->collectedResult.result = make_failure(33);
        h->collectedResult.resultDetails = h->resultDetails;
        return make_failure(33);
    }

    if (progressFn != NULL)
    {
        progressFn(h->stepCtx.stepId, 100, "Execute", "SWUpdate completed successfully", progressCtx);
    }

    ADUC_EXT_LOG_INFO(h->extCtx, SWU_COMP, 3033, "SWUpdate completed successfully");
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 SwuHandler_Validate(ADUC_StepHandle handle)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;

    if (h->cancelled)
    {
        return make_failure(40);
    }

    ADUC_EXT_LOG_INFO(h->extCtx, SWU_COMP, 3040, "Validate: checking installation");

    if (h->config.verifyCommand[0] == '\0')
    {
        /* No verify command — trust swupdate exit code */
        ADUC_EXT_LOG_INFO(h->extCtx, SWU_COMP, 3041, "No verifyCommand; trusting SWUpdate exit code");
        return ADUC_RESULT2_SUCCESS;
    }

    char output[512];
    if (!run_verify_command(h, output, sizeof(output)))
    {
        ADUC_EXT_LOG_ERROR(h->extCtx, SWU_COMP, 3042, "Verify command failed");
        snprintf(h->resultDetails, sizeof(h->resultDetails),
                 "Verify command failed: %s", h->config.verifyCommand);
        h->collectedResult.result = make_failure(41);
        h->collectedResult.resultDetails = h->resultDetails;
        return make_failure(41);
    }

    if (h->config.expectedVersion[0] != '\0')
    {
        if (strcmp(output, h->config.expectedVersion) != 0)
        {
            ADUC_EXT_LOG_ERROR(h->extCtx, SWU_COMP, 3043,
                               "Version mismatch: expected='%s' got='%s'",
                               h->config.expectedVersion, output);
            snprintf(h->resultDetails, sizeof(h->resultDetails),
                     "Version mismatch: expected '%s', got '%s'",
                     h->config.expectedVersion, output);
            h->collectedResult.result = make_failure(42);
            h->collectedResult.resultDetails = h->resultDetails;
            return make_failure(42);
        }
    }

    ADUC_EXT_LOG_INFO(h->extCtx, SWU_COMP, 3044, "Validation passed: version=%s", output);
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 SwuHandler_Postprocess(ADUC_StepHandle handle, bool rollback)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;

    if (rollback)
    {
        ADUC_EXT_LOG_WARN(h->extCtx, SWU_COMP, 3050,
                          "Rollback requested but SWUpdate rollback is device-specific. "
                          "Manual intervention may be required.");
        h->collectedResult.rollbackPerformed = false;
        return ADUC_RESULT2_SUCCESS;
    }

    /* Signal deferred reboot if configured */
    if (h->config.rebootRequired)
    {
        ADUC_EXT_LOG_INFO(h->extCtx, SWU_COMP, 3051, "Signaling deferred reboot");
        h->collectedResult.signal = ADUC_SIGNAL_DEFER_REBOOT;
    }

    return ADUC_RESULT2_SUCCESS;
}

static ADUC_StepResult SwuHandler_GetResult(ADUC_StepHandle handle)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;

    if (ADUC_RESULT2_IS_SUCCESS(h->collectedResult.result))
    {
        h->collectedResult.result = ADUC_RESULT2_SUCCESS;
        snprintf(h->resultDetails, sizeof(h->resultDetails), "SWUpdate operation completed successfully");
        h->collectedResult.resultDetails = h->resultDetails;
    }

    /* Ensure reboot signal is set if needed and operation succeeded */
    if (ADUC_RESULT2_IS_SUCCESS(h->collectedResult.result) && h->config.rebootRequired)
    {
        h->collectedResult.signal = ADUC_SIGNAL_DEFER_REBOOT;
    }
    else if (h->collectedResult.signal == 0)
    {
        h->collectedResult.signal = ADUC_SIGNAL_CONTINUE;
    }

    return h->collectedResult;
}

static ADUC_Result2 SwuHandler_Cancel(ADUC_StepHandle handle)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;
    h->cancelled = true;
    ADUC_EXT_LOG_WARN(h->extCtx, SWU_COMP, 3060, "Cancellation requested");
    return ADUC_RESULT2_SUCCESS;
}

static void SwuHandler_Release(ADUC_StepHandle handle)
{
    if (handle != NULL)
    {
        free(handle);
    }
}

static ADUC_Result2 SwuHandler_IsInstalled(const ADUC_StepContext* ctx, bool* outIsInstalled)
{
    if (outIsInstalled == NULL)
    {
        return ADUC_RESULT2_SUCCESS;
    }

    *outIsInstalled = false;

    if (ctx == NULL || ctx->handlerConfigJson == NULL)
    {
        return make_config_error(50);
    }

    SwuHandlerConfig cfg;
    if (!parse_config(ctx->handlerConfigJson, &cfg))
    {
        return make_config_error(51);
    }

    /* If no verify command, we cannot determine installation status */
    if (cfg.verifyCommand[0] == '\0')
    {
        return ADUC_RESULT2_SUCCESS;
    }

    /* Use a temporary handle-like approach to run verify */
    ADUC_StepHandleImpl tempHandle;
    memset(&tempHandle, 0, sizeof(tempHandle));
    tempHandle.config = cfg;
    tempHandle.extCtx = s_extCtx;

    char output[512];
    if (!run_verify_command(&tempHandle, output, sizeof(output)))
    {
        return ADUC_RESULT2_SUCCESS; /* Cannot verify — assume not installed */
    }

    if (cfg.expectedVersion[0] != '\0')
    {
        *outIsInstalled = (strcmp(output, cfg.expectedVersion) == 0);
    }
    else
    {
        /* Verify command succeeded — assume installed */
        *outIsInstalled = true;
    }

    return ADUC_RESULT2_SUCCESS;
}

/* ─── Vtable ──────────────────────────────────────────────────────────────── */

static const ADUC_StepHandlerVtable s_vtable = {
    .structVersion = 1,
    .Evaluate = SwuHandler_Evaluate,
    .Acquire = SwuHandler_Acquire,
    .Preprocess = SwuHandler_Preprocess,
    .Execute = SwuHandler_Execute,
    .Validate = SwuHandler_Validate,
    .Postprocess = SwuHandler_Postprocess,
    .GetResult = SwuHandler_GetResult,
    .Cancel = SwuHandler_Cancel,
    .Release = SwuHandler_Release,
    .IsInstalled = SwuHandler_IsInstalled,
};

/* ─── Extension lifecycle ─────────────────────────────────────────────────── */

ADUC_Result2 SwuHandler_Initialize(const ADUC_ExtensionContext* ctx)
{
    s_extCtx = ctx;
    ADUC_EXT_LOG_INFO(ctx, SWU_COMP, 3000, "Microsoft SWUpdate Handler v2 initialized");
    return ADUC_RESULT2_SUCCESS;
}

void SwuHandler_Uninitialize(void)
{
    if (s_extCtx != NULL)
    {
        ADUC_EXT_LOG_INFO(s_extCtx, SWU_COMP, 3099, "Microsoft SWUpdate Handler v2 uninitialized");
    }
    s_extCtx = NULL;
}

/* ─── Extension descriptor ────────────────────────────────────────────────── */

static const char* s_capabilities[] = { "microsoft/swupdate:2", "microsoft/swupdate:1", NULL };

static const ADUC_ExtensionDescriptor s_descriptor = {
    .structVersion = ADUC_EXTENSION_DESCRIPTOR_VERSION,
    .id = "microsoft-swupdate-v2",
    .name = "Microsoft SWUpdate Handler v2",
    .version = "2.0.0",
    .type = ADUC_EXT_TYPE_STEP_HANDLER,
    .minHostApiVersion = 1,
    .Initialize = SwuHandler_Initialize,
    .Uninitialize = SwuHandler_Uninitialize,
    .vtable = &s_vtable,
    .capabilities = s_capabilities,
};

ADUC_DECLARE_EXTENSION(s_descriptor)
