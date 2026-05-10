/**
 * @file process_supervisor.c
 * @brief Implementation of the ADU Gen2 process supervisor.
 *
 * Provides child process lifecycle management using POSIX APIs:
 * fork/execve for spawning, pipe for output capture, waitpid for
 * monitoring, kill for termination, and PID-file tracking for
 * orphan cleanup.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/process_supervisor.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef _WIN32
#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <poll.h>
#include <pwd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#define PID_DIR "/var/lib/adu/pids"
#define POLL_INTERVAL_USEC 100000 /* 100 ms */

#ifdef _WIN32
/*
 * Windows stub implementations — process supervisor uses fork/exec/waitpid
 * which are not available on Windows. These stubs compile cleanly but
 * return "not implemented" errors at runtime.
 */

int ADUC_ProcessSupervisor_Spawn(const ADUC_ProcessConfig* config, ADUC_ProcessHandle* outHandle)
{
    (void)config;
    (void)outHandle;
    return -1;
}

int ADUC_ProcessSupervisor_Wait(ADUC_ProcessHandle handle, uint32_t timeoutMs)
{
    (void)handle;
    (void)timeoutMs;
    return -1;
}

bool ADUC_ProcessSupervisor_IsRunning(ADUC_ProcessHandle handle)
{
    (void)handle;
    return false;
}

int ADUC_ProcessSupervisor_GetResult(ADUC_ProcessHandle handle, ADUC_ProcessResult* outResult)
{
    (void)handle;
    if (outResult)
    {
        memset(outResult, 0, sizeof(*outResult));
        outResult->state = ADUC_PROC_STATE_FAILED_TO_START;
    }
    return -1;
}

int ADUC_ProcessSupervisor_Terminate(ADUC_ProcessHandle handle, uint32_t graceMs)
{
    (void)handle;
    (void)graceMs;
    return -1;
}

int ADUC_ProcessSupervisor_Kill(ADUC_ProcessHandle handle)
{
    (void)handle;
    return -1;
}

void ADUC_ProcessSupervisor_FreeResult(ADUC_ProcessResult* result)
{
    if (result)
    {
        free(result->stdoutData);
        free(result->stderrData);
        result->stdoutData = NULL;
        result->stderrData = NULL;
    }
}

void ADUC_ProcessSupervisor_FreeHandle(ADUC_ProcessHandle handle)
{
    (void)handle;
}

int ADUC_ProcessSupervisor_CleanupOrphans(const char* stateDir)
{
    (void)stateDir;
    return 0;
}

int ADUC_ProcessSupervisor_Run(const ADUC_ProcessConfig* config, ADUC_ProcessResult* outResult)
{
    (void)config;
    if (outResult)
    {
        memset(outResult, 0, sizeof(*outResult));
        outResult->state = ADUC_PROC_STATE_FAILED_TO_START;
    }
    return -1;
}

#else /* !_WIN32 */

/* Internal process state */
struct ADUC_Process
{
    pid_t pid;
    ADUC_ProcessState state;
    bool reaped;
    int waitStatus;

    /* Pipe file descriptors for captured output (-1 if unused) */
    int stdoutReadFd;
    int stderrReadFd;

    /* Captured output buffers */
    char* stdoutBuf;
    size_t stdoutLen;
    size_t stdoutCap;
    char* stderrBuf;
    size_t stderrLen;
    size_t stderrCap;
    size_t maxOutputBytes;

    /* Timing */
    double startTime;
    double endTime;

    /* PID file path (for orphan tracking) */
    char* pidFilePath;
};

/* -- Helpers ------------------------------------------------------------ */

static double monotonic_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static char* safe_strdup(const char* s)
{
    return s ? strdup(s) : NULL;
}

static void set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0)
    {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

static void close_fd(int* fd)
{
    if (*fd >= 0)
    {
        close(*fd);
        *fd = -1;
    }
}

/* Append data from fd to buffer, respecting max. Returns false on EOF/error. */
static bool drain_fd(int fd, char** buf, size_t* len, size_t* cap, size_t maxBytes)
{
    if (fd < 0)
    {
        return false;
    }

    char tmp[4096];
    for (;;)
    {
        ssize_t n = read(fd, tmp, sizeof(tmp));
        if (n > 0)
        {
            size_t avail = (maxBytes > *len) ? (maxBytes - *len) : 0;
            size_t toAppend = ((size_t)n < avail) ? (size_t)n : avail;
            if (toAppend > 0)
            {
                size_t needed = *len + toAppend + 1;
                if (needed > *cap)
                {
                    size_t newCap = (*cap == 0) ? 4096 : *cap;
                    while (newCap < needed) { newCap *= 2; }
                    if (newCap > maxBytes + 1) { newCap = maxBytes + 1; }
                    char* nb = realloc(*buf, newCap);
                    if (nb == NULL) { return true; }
                    *buf = nb;
                    *cap = newCap;
                }
                memcpy(*buf + *len, tmp, toAppend);
                *len += toAppend;
                (*buf)[*len] = '\0';
            }
        }
        else if (n == 0)
        {
            return false; /* EOF */
        }
        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return true; /* No data right now */
            }
            return false; /* Error */
        }
    }
}

static void drain_pipes(struct ADUC_Process* proc)
{
    drain_fd(proc->stdoutReadFd, &proc->stdoutBuf, &proc->stdoutLen,
             &proc->stdoutCap, proc->maxOutputBytes);
    drain_fd(proc->stderrReadFd, &proc->stderrBuf, &proc->stderrLen,
             &proc->stderrCap, proc->maxOutputBytes);
}

static bool try_reap(struct ADUC_Process* proc)
{
    if (proc->reaped) { return true; }
    int status = 0;
    pid_t ret = waitpid(proc->pid, &status, WNOHANG);
    if (ret == proc->pid)
    {
        proc->reaped = true;
        proc->waitStatus = status;
        proc->endTime = monotonic_sec();
        return true;
    }
    return false;
}

/* Write PID file for orphan tracking */
static char* write_pid_file(pid_t pid)
{
    struct stat st;
    if (stat(PID_DIR, &st) != 0)
    {
        if (mkdir(PID_DIR, 0755) != 0 && errno != EEXIST)
        {
            return NULL;
        }
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/%d.pid", PID_DIR, (int)pid);

    FILE* f = fopen(path, "w");
    if (f == NULL) { return NULL; }
    fprintf(f, "%d\n", (int)pid);
    fclose(f);
    return safe_strdup(path);
}

static void remove_pid_file(struct ADUC_Process* proc)
{
    if (proc->pidFilePath != NULL)
    {
        unlink(proc->pidFilePath);
        free(proc->pidFilePath);
        proc->pidFilePath = NULL;
    }
}

/* Switch identity in child process */
static int child_switch_identity(const char* userName, const char* groupName)
{
    if (groupName != NULL)
    {
        struct group* grp = getgrnam(groupName);
        if (grp == NULL) { return -1; }
        if (setgid(grp->gr_gid) != 0) { return -1; }
    }

    if (userName != NULL)
    {
        struct passwd* pw = getpwnam(userName);
        if (pw == NULL) { return -1; }
        if (groupName == NULL)
        {
            if (setgid(pw->pw_gid) != 0) { return -1; }
        }
        if (initgroups(userName, pw->pw_gid) != 0) { return -1; }
        if (setuid(pw->pw_uid) != 0) { return -1; }
    }
    return 0;
}

/* Set environment variables from NULL-terminated array of "KEY=VALUE" strings */
static void child_set_envp(const char* const* envp)
{
    if (envp == NULL) { return; }
    for (size_t i = 0; envp[i] != NULL; i++)
    {
        const char* eq = strchr(envp[i], '=');
        if (eq != NULL)
        {
            size_t keyLen = (size_t)(eq - envp[i]);
            char key[256];
            if (keyLen < sizeof(key))
            {
                memcpy(key, envp[i], keyLen);
                key[keyLen] = '\0';
                setenv(key, eq + 1, 1);
            }
        }
    }
}

/* -- Public API --------------------------------------------------------- */

int ADUC_ProcessSupervisor_Spawn(const ADUC_ProcessConfig* config, ADUC_ProcessHandle* outHandle)
{
    if (config == NULL || outHandle == NULL || config->executablePath == NULL || config->argv == NULL)
    {
        return -1;
    }

    *outHandle = NULL;

    struct ADUC_Process* proc = calloc(1, sizeof(*proc));
    if (proc == NULL) { return -2; }

    proc->stdoutReadFd = -1;
    proc->stderrReadFd = -1;
    proc->state = ADUC_PROC_STATE_NOT_STARTED;
    proc->maxOutputBytes = (config->maxOutputBytes > 0)
        ? config->maxOutputBytes : ADUC_PROC_DEFAULT_MAX_OUTPUT;

    int stdoutPipe[2] = { -1, -1 };
    int stderrPipe[2] = { -1, -1 };

    if (config->captureOutput)
    {
        if (pipe(stdoutPipe) != 0 || pipe(stderrPipe) != 0)
        {
            close_fd(&stdoutPipe[0]); close_fd(&stdoutPipe[1]);
            close_fd(&stderrPipe[0]); close_fd(&stderrPipe[1]);
            free(proc);
            return -3;
        }
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        close_fd(&stdoutPipe[0]); close_fd(&stdoutPipe[1]);
        close_fd(&stderrPipe[0]); close_fd(&stderrPipe[1]);
        free(proc);
        return -4;
    }

    if (pid == 0)
    {
        /* -- Child process -- */

        /* Set environment variables */
        child_set_envp(config->envp);

        /* Redirect stdout */
        if (config->captureOutput)
        {
            close(stdoutPipe[0]);
            dup2(stdoutPipe[1], STDOUT_FILENO);
            close(stdoutPipe[1]);
        }
        else if (config->stdoutPath != NULL)
        {
            int fd = open(config->stdoutPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd >= 0) { dup2(fd, STDOUT_FILENO); close(fd); }
        }
        else
        {
            int fd = open("/dev/null", O_WRONLY);
            if (fd >= 0) { dup2(fd, STDOUT_FILENO); close(fd); }
        }

        /* Redirect stderr */
        if (config->captureOutput)
        {
            close(stderrPipe[0]);
            dup2(stderrPipe[1], STDERR_FILENO);
            close(stderrPipe[1]);
        }
        else if (config->stderrPath != NULL)
        {
            int fd = open(config->stderrPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd >= 0) { dup2(fd, STDERR_FILENO); close(fd); }
        }
        else
        {
            int fd = open("/dev/null", O_WRONLY);
            if (fd >= 0) { dup2(fd, STDERR_FILENO); close(fd); }
        }

        /* Switch identity */
        if (config->runAsUser != NULL || config->runAsGroup != NULL)
        {
            if (child_switch_identity(config->runAsUser, config->runAsGroup) != 0)
            {
                _exit(126);
            }
        }

        /* Change working directory */
        if (config->workingDir != NULL)
        {
            if (chdir(config->workingDir) != 0)
            {
                _exit(125);
            }
        }

        execvp(config->executablePath, (char* const*)config->argv);
        _exit(127);
    }

    /* -- Parent process -- */
    proc->pid = pid;
    proc->state = ADUC_PROC_STATE_RUNNING;
    proc->startTime = monotonic_sec();

    if (config->captureOutput)
    {
        close(stdoutPipe[1]);
        close(stderrPipe[1]);
        proc->stdoutReadFd = stdoutPipe[0];
        proc->stderrReadFd = stderrPipe[0];
        set_nonblocking(proc->stdoutReadFd);
        set_nonblocking(proc->stderrReadFd);
    }

    proc->pidFilePath = write_pid_file(pid);

    *outHandle = proc;
    return 0;
}

int ADUC_ProcessSupervisor_Wait(ADUC_ProcessHandle handle, uint32_t timeoutMs)
{
    if (handle == NULL) { return -1; }

    struct ADUC_Process* proc = handle;
    if (proc->state != ADUC_PROC_STATE_RUNNING) { return 0; }

    double deadline = 0;
    if (timeoutMs > 0)
    {
        deadline = monotonic_sec() + (double)timeoutMs / 1000.0;
    }

    for (;;)
    {
        drain_pipes(proc);

        if (try_reap(proc))
        {
            /* Final drain after reap */
            drain_pipes(proc);
            close_fd(&proc->stdoutReadFd);
            close_fd(&proc->stderrReadFd);

            /* Determine state from wait status */
            if (WIFEXITED(proc->waitStatus))
            {
                int code = WEXITSTATUS(proc->waitStatus);
                if (code == 127 || code == 126 || code == 125)
                {
                    proc->state = ADUC_PROC_STATE_FAILED_TO_START;
                }
                else
                {
                    proc->state = ADUC_PROC_STATE_COMPLETED;
                }
            }
            else if (WIFSIGNALED(proc->waitStatus))
            {
                proc->state = ADUC_PROC_STATE_SIGNALED;
            }
            else
            {
                proc->state = ADUC_PROC_STATE_COMPLETED;
            }

            remove_pid_file(proc);
            return 0;
        }

        if (deadline > 0 && monotonic_sec() >= deadline)
        {
            /* Timeout: terminate the child */
            ADUC_ProcessSupervisor_Terminate(handle, 5000);
            proc->state = ADUC_PROC_STATE_TIMED_OUT;
            proc->endTime = monotonic_sec();
            close_fd(&proc->stdoutReadFd);
            close_fd(&proc->stderrReadFd);
            remove_pid_file(proc);
            return 0;
        }

        usleep(POLL_INTERVAL_USEC);
    }
}

bool ADUC_ProcessSupervisor_IsRunning(ADUC_ProcessHandle handle)
{
    if (handle == NULL) { return false; }
    struct ADUC_Process* proc = handle;
    if (proc->state != ADUC_PROC_STATE_RUNNING) { return false; }

    drain_pipes(proc);

    if (try_reap(proc))
    {
        drain_pipes(proc);
        close_fd(&proc->stdoutReadFd);
        close_fd(&proc->stderrReadFd);

        if (WIFEXITED(proc->waitStatus))
        {
            int code = WEXITSTATUS(proc->waitStatus);
            proc->state = (code == 127 || code == 126 || code == 125)
                ? ADUC_PROC_STATE_FAILED_TO_START
                : ADUC_PROC_STATE_COMPLETED;
        }
        else if (WIFSIGNALED(proc->waitStatus))
        {
            proc->state = ADUC_PROC_STATE_SIGNALED;
        }
        else
        {
            proc->state = ADUC_PROC_STATE_COMPLETED;
        }
        remove_pid_file(proc);
        return false;
    }
    return true;
}

int ADUC_ProcessSupervisor_GetResult(ADUC_ProcessHandle handle, ADUC_ProcessResult* outResult)
{
    if (handle == NULL || outResult == NULL) { return -1; }

    struct ADUC_Process* proc = handle;
    memset(outResult, 0, sizeof(*outResult));

    outResult->state = proc->state;
    outResult->elapsedSeconds = (proc->endTime > 0)
        ? (proc->endTime - proc->startTime)
        : (monotonic_sec() - proc->startTime);

    if (proc->state == ADUC_PROC_STATE_COMPLETED || proc->state == ADUC_PROC_STATE_FAILED_TO_START)
    {
        outResult->exitCode = WIFEXITED(proc->waitStatus) ? WEXITSTATUS(proc->waitStatus) : -1;
    }
    else if (proc->state == ADUC_PROC_STATE_SIGNALED)
    {
        outResult->signalNumber = WIFSIGNALED(proc->waitStatus) ? WTERMSIG(proc->waitStatus) : 0;
        outResult->exitCode = -1;
    }
    else if (proc->state == ADUC_PROC_STATE_TIMED_OUT)
    {
        outResult->exitCode = -1;
    }

    /* Transfer ownership of captured output */
    outResult->stdoutData = proc->stdoutBuf;
    outResult->stdoutLen = proc->stdoutLen;
    outResult->stderrData = proc->stderrBuf;
    outResult->stderrLen = proc->stderrLen;

    proc->stdoutBuf = NULL;
    proc->stdoutLen = 0;
    proc->stdoutCap = 0;
    proc->stderrBuf = NULL;
    proc->stderrLen = 0;
    proc->stderrCap = 0;

    return 0;
}

int ADUC_ProcessSupervisor_Terminate(ADUC_ProcessHandle handle, uint32_t graceMs)
{
    if (handle == NULL) { return -1; }

    struct ADUC_Process* proc = handle;
    if (proc->reaped) { return 0; }

    /* Send SIGTERM */
    if (kill(proc->pid, SIGTERM) != 0 && errno != ESRCH) { return -2; }

    /* Wait grace period */
    double graceEnd = monotonic_sec() + (double)graceMs / 1000.0;
    while (monotonic_sec() < graceEnd)
    {
        if (try_reap(proc))
        {
            remove_pid_file(proc);
            return 0;
        }
        usleep(POLL_INTERVAL_USEC);
    }

    /* SIGKILL if still alive */
    return ADUC_ProcessSupervisor_Kill(handle);
}

int ADUC_ProcessSupervisor_Kill(ADUC_ProcessHandle handle)
{
    if (handle == NULL) { return -1; }

    struct ADUC_Process* proc = handle;
    if (proc->reaped) { return 0; }

    if (kill(proc->pid, SIGKILL) != 0 && errno != ESRCH) { return -2; }

    int status = 0;
    waitpid(proc->pid, &status, 0);
    proc->reaped = true;
    proc->waitStatus = status;
    proc->endTime = monotonic_sec();
    remove_pid_file(proc);

    return 0;
}

void ADUC_ProcessSupervisor_FreeResult(ADUC_ProcessResult* result)
{
    if (result == NULL) { return; }
    free(result->stdoutData);
    result->stdoutData = NULL;
    result->stdoutLen = 0;
    free(result->stderrData);
    result->stderrData = NULL;
    result->stderrLen = 0;
}

void ADUC_ProcessSupervisor_FreeHandle(ADUC_ProcessHandle handle)
{
    if (handle == NULL) { return; }

    struct ADUC_Process* proc = handle;

    /* Kill if still running */
    if (!proc->reaped)
    {
        ADUC_ProcessSupervisor_Terminate(handle, 5000);
    }

    close_fd(&proc->stdoutReadFd);
    close_fd(&proc->stderrReadFd);
    free(proc->stdoutBuf);
    free(proc->stderrBuf);
    free(proc->pidFilePath);
    free(proc);
}

int ADUC_ProcessSupervisor_CleanupOrphans(const char* stateDir)
{
    const char* dir = (stateDir != NULL) ? stateDir : PID_DIR;

    DIR* d = opendir(dir);
    if (d == NULL) { return (errno == ENOENT) ? 0 : -1; }

    struct dirent* ent;
    while ((ent = readdir(d)) != NULL)
    {
        /* Only process .pid files */
        const char* ext = strrchr(ent->d_name, '.');
        if (ext == NULL || strcmp(ext, ".pid") != 0) { continue; }

        char path[512];
        snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);

        FILE* f = fopen(path, "r");
        if (f == NULL) { continue; }

        int pid = 0;
        if (fscanf(f, "%d", &pid) != 1 || pid <= 0)
        {
            fclose(f);
            unlink(path);
            continue;
        }
        fclose(f);

        /* Check if process still exists */
        if (kill((pid_t)pid, 0) == 0)
        {
            /* Verify via /proc/<pid>/cmdline */
            char procPath[64];
            snprintf(procPath, sizeof(procPath), "/proc/%d/cmdline", pid);
            if (access(procPath, F_OK) == 0)
            {
                kill((pid_t)pid, SIGKILL);
                waitpid((pid_t)pid, NULL, WNOHANG);
            }
        }

        unlink(path);
    }

    closedir(d);
    return 0;
}

int ADUC_ProcessSupervisor_Run(const ADUC_ProcessConfig* config, ADUC_ProcessResult* outResult)
{
    if (outResult == NULL) { return -1; }
    memset(outResult, 0, sizeof(*outResult));

    ADUC_ProcessHandle handle = NULL;
    int rc = ADUC_ProcessSupervisor_Spawn(config, &handle);
    if (rc != 0)
    {
        outResult->state = ADUC_PROC_STATE_FAILED_TO_START;
        outResult->exitCode = -1;
        return rc;
    }

    uint32_t waitMs = 0;
    if (config->timeoutSeconds > 0)
    {
        waitMs = config->timeoutSeconds * 1000u;
    }

    rc = ADUC_ProcessSupervisor_Wait(handle, waitMs);
    if (rc != 0)
    {
        ADUC_ProcessSupervisor_FreeHandle(handle);
        return rc;
    }

    rc = ADUC_ProcessSupervisor_GetResult(handle, outResult);
    ADUC_ProcessSupervisor_FreeHandle(handle);
    return rc;
}

#endif /* !_WIN32 */
