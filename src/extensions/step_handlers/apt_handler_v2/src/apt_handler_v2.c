/**
 * @file apt_handler_v2.c
 * @brief Microsoft APT Package Handler v2 — installs, upgrades, or removes
 *        Debian packages via apt-get with rollback support.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "apt_handler_v2.h"

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

#define APT_COMP "microsoft-apt-v2"

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

        /* Set read end to non-blocking for timeout support */
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
                    /* Check if child has exited */
                    int status = 0;
                    pid_t w = waitpid(pid, &status, WNOHANG);
                    if (w == pid)
                    {
                        /* Drain remaining output */
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

    /* Wait for child or kill on timeout */
    int status = 0;
    pid_t w = waitpid(pid, &status, WNOHANG);
    if (w == 0)
    {
        /* Still running — check if we timed out */
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

/* ─── APT environment setup ───────────────────────────────────────────────── */

static char* s_aptEnv[] = {
    "DEBIAN_FRONTEND=noninteractive",
    "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin",
    NULL
};

/* ─── JSON config parsing ─────────────────────────────────────────────────── */

static bool parse_config(const char* json, AptHandlerConfig* cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    strncpy(cfg->action, "install", APT_MAX_ACTION_LEN - 1);

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

    /* Parse packages array (required) */
    JSON_Array* pkgArray = json_object_get_array(root, "packages");
    if (pkgArray == NULL)
    {
        json_value_free(rootVal);
        return false;
    }

    size_t count = json_array_get_count(pkgArray);
    if (count == 0 || count > APT_MAX_PACKAGES)
    {
        json_value_free(rootVal);
        return false;
    }

    for (size_t i = 0; i < count; i++)
    {
        const char* pkg = json_array_get_string(pkgArray, i);
        if (pkg == NULL || pkg[0] == '\0')
        {
            /* Free already allocated strings */
            for (size_t j = 0; j < cfg->packageCount; j++)
            {
                free(cfg->packages[j]);
                cfg->packages[j] = NULL;
            }
            json_value_free(rootVal);
            return false;
        }
        cfg->packages[i] = strdup(pkg);
        if (cfg->packages[i] == NULL)
        {
            for (size_t j = 0; j < i; j++)
            {
                free(cfg->packages[j]);
                cfg->packages[j] = NULL;
            }
            json_value_free(rootVal);
            return false;
        }
        cfg->packageCount++;
    }

    /* Parse action (optional) */
    const char* action = json_object_get_string(root, "action");
    if (action != NULL && action[0] != '\0')
    {
        strncpy(cfg->action, action, APT_MAX_ACTION_LEN - 1);
        cfg->action[APT_MAX_ACTION_LEN - 1] = '\0';
    }

    /* Parse options (optional) */
    const char* opts = json_object_get_string(root, "options");
    if (opts != NULL)
    {
        strncpy(cfg->options, opts, APT_MAX_OPTION_LEN - 1);
        cfg->options[APT_MAX_OPTION_LEN - 1] = '\0';
    }

    /* Parse sourceList (optional) */
    const char* src = json_object_get_string(root, "sourceList");
    if (src != NULL)
    {
        strncpy(cfg->sourceList, src, APT_MAX_PATH_LEN - 1);
        cfg->sourceList[APT_MAX_PATH_LEN - 1] = '\0';
    }

    json_value_free(rootVal);
    return true;
}

static void free_config(AptHandlerConfig* cfg)
{
    for (size_t i = 0; i < cfg->packageCount; i++)
    {
        free(cfg->packages[i]);
        cfg->packages[i] = NULL;
    }
    cfg->packageCount = 0;
}

/* ─── Package version query helper ────────────────────────────────────────── */

static bool get_package_version(const char* packageName, char* versionBuf, size_t bufSize, bool* isInstalled)
{
    char output[1024];
    char* argv[] = { "/usr/bin/dpkg-query", "-W", "-f", "${Status} ${Version}", (char*)packageName, NULL };

    int rc = run_command("/usr/bin/dpkg-query", argv, NULL, output, sizeof(output), 30);
    if (rc != 0)
    {
        if (isInstalled != NULL)
        {
            *isInstalled = false;
        }
        if (versionBuf != NULL)
        {
            versionBuf[0] = '\0';
        }
        return true; /* Not installed is a valid state */
    }

    /* Parse "install ok installed X.Y.Z" or similar */
    if (isInstalled != NULL)
    {
        *isInstalled = (strstr(output, "install ok installed") != NULL);
    }

    if (versionBuf != NULL)
    {
        const char* verStart = strrchr(output, ' ');
        if (verStart != NULL)
        {
            verStart++;
            strncpy(versionBuf, verStart, bufSize - 1);
            versionBuf[bufSize - 1] = '\0';
            /* Trim trailing newline */
            size_t len = strlen(versionBuf);
            while (len > 0 && (versionBuf[len - 1] == '\n' || versionBuf[len - 1] == '\r'))
            {
                versionBuf[--len] = '\0';
            }
        }
        else
        {
            versionBuf[0] = '\0';
        }
    }

    return true;
}

/* ─── Vtable implementations ─────────────────────────────────────────────── */

static ADUC_Result2 AptHandler_Evaluate(const ADUC_StepContext* ctx, ADUC_StepHandle* handle)
{
    ADUC_EXT_LOG_INFO(s_extCtx, APT_COMP, 2001, "Evaluating APT step %s", ctx->stepId ? ctx->stepId : "(null)");

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
        ADUC_EXT_LOG_ERROR(s_extCtx, APT_COMP, 2002, "Failed to parse handlerProperties");
        snprintf(h->resultDetails, sizeof(h->resultDetails), "Invalid handlerProperties JSON");
        h->collectedResult.result = make_config_error(1);
        h->collectedResult.resultDetails = h->resultDetails;
        *handle = h;
        return make_config_error(1);
    }

    if (h->config.packageCount == 0)
    {
        ADUC_EXT_LOG_ERROR(s_extCtx, APT_COMP, 2003, "No packages specified");
        snprintf(h->resultDetails, sizeof(h->resultDetails), "No packages specified in handlerProperties");
        h->collectedResult.result = make_config_error(2);
        h->collectedResult.resultDetails = h->resultDetails;
        *handle = h;
        return make_config_error(2);
    }

    /* Validate action */
    if (strcmp(h->config.action, "install") != 0 &&
        strcmp(h->config.action, "upgrade") != 0 &&
        strcmp(h->config.action, "remove") != 0 &&
        strcmp(h->config.action, "dist-upgrade") != 0)
    {
        ADUC_EXT_LOG_ERROR(s_extCtx, APT_COMP, 2004, "Invalid action: %s", h->config.action);
        snprintf(h->resultDetails, sizeof(h->resultDetails), "Invalid action '%s'", h->config.action);
        h->collectedResult.result = make_config_error(3);
        h->collectedResult.resultDetails = h->resultDetails;
        *handle = h;
        return make_config_error(3);
    }

    ADUC_EXT_LOG_INFO(
        s_extCtx, APT_COMP, 2005,
        "APT config: action=%s, packages=%zu", h->config.action, h->config.packageCount);

    *handle = h;
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 AptHandler_Acquire(ADUC_StepHandle handle)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;

    if (h->cancelled)
    {
        return make_failure(10);
    }

    ADUC_EXT_LOG_INFO(h->extCtx, APT_COMP, 2010, "Acquire: checking sourceList");

    /* If sourceList specified, find matching .list file in step files and copy it */
    if (h->config.sourceList[0] != '\0')
    {
        const ADUC_StepFile* listFile = NULL;
        for (size_t i = 0; i < h->stepCtx.fileCount; i++)
        {
            const char* fname = h->stepCtx.files[i].name;
            size_t len = strlen(fname);
            if (len > 5 && strcmp(fname + len - 5, ".list") == 0)
            {
                listFile = &h->stepCtx.files[i];
                break;
            }
        }

        if (listFile != NULL && listFile->path != NULL)
        {
            ADUC_EXT_LOG_INFO(h->extCtx, APT_COMP, 2011, "Copying sources list to %s", h->config.sourceList);

            /* Copy file to sources.list.d */
            FILE* src = fopen(listFile->path, "r");
            if (src == NULL)
            {
                ADUC_EXT_LOG_ERROR(h->extCtx, APT_COMP, 2012, "Cannot open source list file: %s", listFile->path);
                snprintf(h->resultDetails, sizeof(h->resultDetails),
                         "Failed to open sources list: %s", listFile->path);
                h->collectedResult.result = make_failure(11);
                h->collectedResult.resultDetails = h->resultDetails;
                return make_failure(11);
            }

            FILE* dst = fopen(h->config.sourceList, "w");
            if (dst == NULL)
            {
                fclose(src);
                ADUC_EXT_LOG_ERROR(h->extCtx, APT_COMP, 2013, "Cannot write to %s", h->config.sourceList);
                snprintf(h->resultDetails, sizeof(h->resultDetails),
                         "Failed to write sources list: %s", h->config.sourceList);
                h->collectedResult.result = make_failure(12);
                h->collectedResult.resultDetails = h->resultDetails;
                return make_failure(12);
            }

            char buf[4096];
            size_t n;
            while ((n = fread(buf, 1, sizeof(buf), src)) > 0)
            {
                if (fwrite(buf, 1, n, dst) != n)
                {
                    fclose(src);
                    fclose(dst);
                    h->collectedResult.result = make_failure(13);
                    h->collectedResult.resultDetails = "Failed to write sources list content";
                    return make_failure(13);
                }
            }
            fclose(src);
            fclose(dst);
        }

        /* Run apt-get update */
        ADUC_EXT_LOG_INFO(h->extCtx, APT_COMP, 2014, "Running apt-get update");
        char output[APT_OUTPUT_BUF_SIZE];
        char* argv[] = { "/usr/bin/apt-get", "update", "-qq", NULL };
        int rc = run_command("/usr/bin/apt-get", argv, s_aptEnv, output, sizeof(output), APT_DEFAULT_TIMEOUT_SEC);
        if (rc != 0)
        {
            ADUC_EXT_LOG_WARN(h->extCtx, APT_COMP, 2015, "apt-get update returned %d: %s", rc, output);
            /* Non-fatal: index update failure may still allow install from cache */
        }
    }

    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 AptHandler_Preprocess(ADUC_StepHandle handle)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;

    if (h->cancelled)
    {
        return make_failure(20);
    }

    ADUC_EXT_LOG_INFO(h->extCtx, APT_COMP, 2020, "Preprocess: recording current package versions");

    /* Record current versions for rollback */
    h->originalVersionCount = 0;
    for (size_t i = 0; i < h->config.packageCount; i++)
    {
        /* Extract base package name (strip version pin like "=2.1.0") */
        char baseName[APT_VERSION_LEN];
        strncpy(baseName, h->config.packages[i], sizeof(baseName) - 1);
        baseName[sizeof(baseName) - 1] = '\0';
        char* eqSign = strchr(baseName, '=');
        if (eqSign != NULL)
        {
            *eqSign = '\0';
        }

        AptPackageVersion* pv = &h->originalVersions[h->originalVersionCount];
        strncpy(pv->name, baseName, APT_VERSION_LEN - 1);
        pv->name[APT_VERSION_LEN - 1] = '\0';

        get_package_version(baseName, pv->version, APT_VERSION_LEN, &pv->wasInstalled);

        ADUC_EXT_LOG_INFO(
            h->extCtx, APT_COMP, 2021,
            "Package %s: installed=%d version=%s",
            pv->name, (int)pv->wasInstalled, pv->version);

        h->originalVersionCount++;
    }

    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 AptHandler_Execute(ADUC_StepHandle handle, ADUC_StepProgressFn progressFn, void* progressCtx)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;

    if (h->cancelled)
    {
        return make_failure(30);
    }

    ADUC_EXT_LOG_INFO(h->extCtx, APT_COMP, 2030, "Execute: running apt-get %s", h->config.action);

    if (progressFn != NULL)
    {
        progressFn(h->stepCtx.stepId, 0, "Execute", "Starting apt-get operation", progressCtx);
    }

    /*
     * Build argv: apt-get -y [action] [options...] [packages...]
     * Max args: "apt-get" + "-y" + action + options(split) + packages + NULL
     */
    char* argv[APT_MAX_PACKAGES + 16];
    int argc = 0;

    argv[argc++] = "/usr/bin/apt-get";
    argv[argc++] = "-y";
    argv[argc++] = h->config.action;

    /* Add options if specified (split by space) */
    char optsCopy[APT_MAX_OPTION_LEN];
    if (h->config.options[0] != '\0')
    {
        strncpy(optsCopy, h->config.options, sizeof(optsCopy) - 1);
        optsCopy[sizeof(optsCopy) - 1] = '\0';
        char* saveptr = NULL;
        char* tok = strtok_r(optsCopy, " ", &saveptr);
        while (tok != NULL && argc < (int)(sizeof(argv) / sizeof(argv[0])) - (int)h->config.packageCount - 1)
        {
            argv[argc++] = tok;
            tok = strtok_r(NULL, " ", &saveptr);
        }
    }

    /* Add packages (skip for dist-upgrade which doesn't take package args) */
    if (strcmp(h->config.action, "dist-upgrade") != 0)
    {
        for (size_t i = 0; i < h->config.packageCount; i++)
        {
            if (argc < (int)(sizeof(argv) / sizeof(argv[0])) - 1)
            {
                argv[argc++] = h->config.packages[i];
            }
        }
    }
    argv[argc] = NULL;

    if (progressFn != NULL)
    {
        progressFn(h->stepCtx.stepId, 10, "Execute", "Running apt-get", progressCtx);
    }

    char output[APT_OUTPUT_BUF_SIZE];
    int rc = run_command("/usr/bin/apt-get", argv, s_aptEnv, output, sizeof(output), APT_DEFAULT_TIMEOUT_SEC);

    if (h->cancelled)
    {
        return make_failure(31);
    }

    if (rc == -2)
    {
        ADUC_EXT_LOG_ERROR(h->extCtx, APT_COMP, 2031, "apt-get timed out after %ds", APT_DEFAULT_TIMEOUT_SEC);
        snprintf(h->resultDetails, sizeof(h->resultDetails), "apt-get timed out after %ds", APT_DEFAULT_TIMEOUT_SEC);
        h->collectedResult.result = make_failure(32);
        h->collectedResult.resultDetails = h->resultDetails;
        return make_failure(32);
    }

    if (rc != 0)
    {
        ADUC_EXT_LOG_ERROR(h->extCtx, APT_COMP, 2032, "apt-get %s failed (rc=%d): %.200s",
                           h->config.action, rc, output);
        snprintf(h->resultDetails, sizeof(h->resultDetails),
                 "apt-get %s failed (exit code %d): %.1800s", h->config.action, rc, output);
        h->collectedResult.result = make_failure(33);
        h->collectedResult.resultDetails = h->resultDetails;
        return make_failure(33);
    }

    if (progressFn != NULL)
    {
        progressFn(h->stepCtx.stepId, 90, "Execute", "apt-get completed successfully", progressCtx);
    }

    ADUC_EXT_LOG_INFO(h->extCtx, APT_COMP, 2033, "apt-get %s completed successfully", h->config.action);
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 AptHandler_Validate(ADUC_StepHandle handle)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;

    if (h->cancelled)
    {
        return make_failure(40);
    }

    ADUC_EXT_LOG_INFO(h->extCtx, APT_COMP, 2040, "Validate: verifying package installation");

    /* For remove action, verify packages are NOT installed */
    bool expectInstalled = (strcmp(h->config.action, "remove") != 0);

    for (size_t i = 0; i < h->config.packageCount; i++)
    {
        /* Extract base name */
        char baseName[APT_VERSION_LEN];
        strncpy(baseName, h->config.packages[i], sizeof(baseName) - 1);
        baseName[sizeof(baseName) - 1] = '\0';
        char* eqSign = strchr(baseName, '=');
        if (eqSign != NULL)
        {
            *eqSign = '\0';
        }

        char output[1024];
        char* argv[] = { "/usr/bin/dpkg", "-s", baseName, NULL };
        int rc = run_command("/usr/bin/dpkg", argv, NULL, output, sizeof(output), 30);

        bool isInstalled = (rc == 0 && strstr(output, "Status: install ok installed") != NULL);

        if (expectInstalled && !isInstalled)
        {
            ADUC_EXT_LOG_ERROR(h->extCtx, APT_COMP, 2041, "Package %s not installed after operation", baseName);
            snprintf(h->resultDetails, sizeof(h->resultDetails),
                     "Validation failed: package '%s' is not installed", baseName);
            h->collectedResult.result = make_failure(41);
            h->collectedResult.resultDetails = h->resultDetails;
            return make_failure(41);
        }

        if (!expectInstalled && isInstalled)
        {
            ADUC_EXT_LOG_ERROR(h->extCtx, APT_COMP, 2042, "Package %s still installed after remove", baseName);
            snprintf(h->resultDetails, sizeof(h->resultDetails),
                     "Validation failed: package '%s' still installed after removal", baseName);
            h->collectedResult.result = make_failure(42);
            h->collectedResult.resultDetails = h->resultDetails;
            return make_failure(42);
        }
    }

    ADUC_EXT_LOG_INFO(h->extCtx, APT_COMP, 2043, "All packages validated successfully");
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 AptHandler_Postprocess(ADUC_StepHandle handle, bool rollback)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;

    if (!rollback)
    {
        ADUC_EXT_LOG_INFO(h->extCtx, APT_COMP, 2050, "Postprocess: no rollback needed");
        return ADUC_RESULT2_SUCCESS;
    }

    ADUC_EXT_LOG_WARN(h->extCtx, APT_COMP, 2051, "Postprocess: performing rollback");
    h->collectedResult.rollbackPerformed = true;

    /* Attempt to restore original package versions */
    for (size_t i = 0; i < h->originalVersionCount; i++)
    {
        const AptPackageVersion* pv = &h->originalVersions[i];

        if (!pv->wasInstalled)
        {
            /* Package wasn't installed before — remove it */
            ADUC_EXT_LOG_INFO(h->extCtx, APT_COMP, 2052, "Rollback: removing %s", pv->name);
            char* argv[] = { "/usr/bin/apt-get", "-y", "remove", (char*)pv->name, NULL };
            char output[APT_OUTPUT_BUF_SIZE];
            int rc = run_command("/usr/bin/apt-get", argv, s_aptEnv, output, sizeof(output), APT_DEFAULT_TIMEOUT_SEC);
            if (rc != 0)
            {
                ADUC_EXT_LOG_WARN(h->extCtx, APT_COMP, 2053, "Rollback remove of %s failed (rc=%d)", pv->name, rc);
            }
        }
        else if (pv->version[0] != '\0')
        {
            /* Reinstall original version */
            char pkgSpec[APT_VERSION_LEN * 2];
            snprintf(pkgSpec, sizeof(pkgSpec), "%s=%s", pv->name, pv->version);
            ADUC_EXT_LOG_INFO(h->extCtx, APT_COMP, 2054, "Rollback: installing %s", pkgSpec);
            char* argv[] = { "/usr/bin/apt-get", "-y", "install", pkgSpec, NULL };
            char output[APT_OUTPUT_BUF_SIZE];
            int rc = run_command("/usr/bin/apt-get", argv, s_aptEnv, output, sizeof(output), APT_DEFAULT_TIMEOUT_SEC);
            if (rc != 0)
            {
                ADUC_EXT_LOG_WARN(h->extCtx, APT_COMP, 2055, "Rollback install of %s failed (rc=%d)", pkgSpec, rc);
            }
        }
    }

    /* Cleanup sourceList if we added one */
    if (h->config.sourceList[0] != '\0')
    {
        ADUC_EXT_LOG_INFO(h->extCtx, APT_COMP, 2056, "Rollback: removing sources list %s", h->config.sourceList);
        (void)unlink(h->config.sourceList);
    }

    return ADUC_RESULT2_SUCCESS;
}

static ADUC_StepResult AptHandler_GetResult(ADUC_StepHandle handle)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;

    if (ADUC_RESULT2_IS_SUCCESS(h->collectedResult.result))
    {
        h->collectedResult.result = ADUC_RESULT2_SUCCESS;
        snprintf(h->resultDetails, sizeof(h->resultDetails), "APT operation completed successfully");
        h->collectedResult.resultDetails = h->resultDetails;
    }

    h->collectedResult.signal = ADUC_SIGNAL_CONTINUE;
    return h->collectedResult;
}

static ADUC_Result2 AptHandler_Cancel(ADUC_StepHandle handle)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;
    h->cancelled = true;
    ADUC_EXT_LOG_WARN(h->extCtx, APT_COMP, 2060, "Cancellation requested");
    return ADUC_RESULT2_SUCCESS;
}

static void AptHandler_Release(ADUC_StepHandle handle)
{
    if (handle != NULL)
    {
        ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;
        free_config(&h->config);
        free(h);
    }
}

static ADUC_Result2 AptHandler_IsInstalled(const ADUC_StepContext* ctx, bool* outIsInstalled)
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

    AptHandlerConfig cfg;
    if (!parse_config(ctx->handlerConfigJson, &cfg))
    {
        return make_config_error(51);
    }

    /* Check if all packages are installed (at expected versions if pinned) */
    bool allInstalled = true;
    for (size_t i = 0; i < cfg.packageCount; i++)
    {
        char baseName[APT_VERSION_LEN];
        char pinnedVersion[APT_VERSION_LEN] = { 0 };

        strncpy(baseName, cfg.packages[i], sizeof(baseName) - 1);
        baseName[sizeof(baseName) - 1] = '\0';
        char* eqSign = strchr(baseName, '=');
        if (eqSign != NULL)
        {
            *eqSign = '\0';
            strncpy(pinnedVersion, eqSign + 1, sizeof(pinnedVersion) - 1);
            pinnedVersion[sizeof(pinnedVersion) - 1] = '\0';
        }

        char currentVersion[APT_VERSION_LEN];
        bool isInstalled = false;
        get_package_version(baseName, currentVersion, sizeof(currentVersion), &isInstalled);

        if (!isInstalled)
        {
            allInstalled = false;
            break;
        }

        if (pinnedVersion[0] != '\0' && strcmp(currentVersion, pinnedVersion) != 0)
        {
            allInstalled = false;
            break;
        }
    }

    free_config(&cfg);
    *outIsInstalled = allInstalled;
    return ADUC_RESULT2_SUCCESS;
}

/* ─── Vtable ──────────────────────────────────────────────────────────────── */

static const ADUC_StepHandlerVtable s_vtable = {
    .structVersion = 1,
    .Evaluate = AptHandler_Evaluate,
    .Acquire = AptHandler_Acquire,
    .Preprocess = AptHandler_Preprocess,
    .Execute = AptHandler_Execute,
    .Validate = AptHandler_Validate,
    .Postprocess = AptHandler_Postprocess,
    .GetResult = AptHandler_GetResult,
    .Cancel = AptHandler_Cancel,
    .Release = AptHandler_Release,
    .IsInstalled = AptHandler_IsInstalled,
};

/* ─── Extension lifecycle ─────────────────────────────────────────────────── */

ADUC_Result2 AptHandler_Initialize(const ADUC_ExtensionContext* ctx)
{
    s_extCtx = ctx;
    ADUC_EXT_LOG_INFO(ctx, APT_COMP, 2000, "Microsoft APT Package Handler v2 initialized");
    return ADUC_RESULT2_SUCCESS;
}

void AptHandler_Uninitialize(void)
{
    if (s_extCtx != NULL)
    {
        ADUC_EXT_LOG_INFO(s_extCtx, APT_COMP, 2099, "Microsoft APT Package Handler v2 uninitialized");
    }
    s_extCtx = NULL;
}

/* ─── Extension descriptor ────────────────────────────────────────────────── */

static const char* s_capabilities[] = { "microsoft/apt:2", "microsoft/apt:1", NULL };

static const ADUC_ExtensionDescriptor s_descriptor = {
    .structVersion = ADUC_EXTENSION_DESCRIPTOR_VERSION,
    .id = "microsoft-apt-v2",
    .name = "Microsoft APT Package Handler v2",
    .version = "2.0.0",
    .type = ADUC_EXT_TYPE_STEP_HANDLER,
    .minHostApiVersion = 1,
    .Initialize = AptHandler_Initialize,
    .Uninitialize = AptHandler_Uninitialize,
    .vtable = &s_vtable,
    .capabilities = s_capabilities,
};

ADUC_DECLARE_EXTENSION(s_descriptor)
