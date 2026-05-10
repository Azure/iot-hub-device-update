/**
 * @file process_supervisor.h
 * @brief Process supervisor for managing child processes spawned by step handlers.
 *
 * Provides lifecycle management for child processes including:
 * - Spawning with specific user/group identity (run-as-user)
 * - stdout/stderr capture (in-memory or file redirect)
 * - Monitoring with timeout enforcement
 * - Graceful termination (SIGTERM → grace period → SIGKILL)
 * - Orphan cleanup via PID file tracking
 * - Synchronous convenience wrapper
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_PROCESS_SUPERVISOR_H
#define ADUC_PROCESS_SUPERVISOR_H

#include "aduc/extension_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque handle to a supervised child process. */
typedef struct ADUC_Process* ADUC_ProcessHandle;

/** Default max captured output: 1 MB */
#define ADUC_PROC_DEFAULT_MAX_OUTPUT (1u << 20)

/**
 * @brief Configuration for spawning a child process.
 */
typedef struct ADUC_ProcessConfig {
    const char* executablePath;         /**< Path to executable or script */
    const char* const* argv;            /**< NULL-terminated argument array */
    const char* const* envp;            /**< NULL-terminated env vars, NULL = inherit */
    const char* workingDir;             /**< Working directory (NULL = inherit) */
    const char* runAsUser;              /**< Run-as user (NULL = same user) */
    const char* runAsGroup;             /**< Run-as group (NULL = same group) */
    const char* stdoutPath;             /**< Redirect stdout to file, NULL = /dev/null */
    const char* stderrPath;             /**< Redirect stderr to file, NULL = /dev/null */
    uint32_t timeoutSeconds;            /**< Max execution time (0 = no limit) */
    bool captureOutput;                 /**< Capture stdout/stderr in memory */
    size_t maxOutputBytes;              /**< Max captured output (0 = default 1 MB) */
} ADUC_ProcessConfig;

/**
 * @brief Process state after completion.
 */
typedef enum ADUC_ProcessState {
    ADUC_PROC_STATE_NOT_STARTED = 0,
    ADUC_PROC_STATE_RUNNING,
    ADUC_PROC_STATE_COMPLETED,
    ADUC_PROC_STATE_TIMED_OUT,
    ADUC_PROC_STATE_SIGNALED,
    ADUC_PROC_STATE_FAILED_TO_START,
} ADUC_ProcessState;

/**
 * @brief Result collected after a child process terminates.
 */
typedef struct ADUC_ProcessResult {
    ADUC_ProcessState state;
    int exitCode;                       /**< Valid only if state == COMPLETED */
    int signalNumber;                   /**< Valid only if state == SIGNALED */
    char* stdoutData;                   /**< Captured stdout (caller frees) */
    size_t stdoutLen;
    char* stderrData;                   /**< Captured stderr (caller frees) */
    size_t stderrLen;
    double elapsedSeconds;
} ADUC_ProcessResult;

/**
 * @brief Spawn a child process.
 * @return 0 on success, negative on error.
 */
int ADUC_ProcessSupervisor_Spawn(const ADUC_ProcessConfig* config, ADUC_ProcessHandle* outHandle);

/**
 * @brief Wait for process to complete (blocking, up to timeoutMs, 0 = forever).
 * @return 0 on success, negative on error.
 */
int ADUC_ProcessSupervisor_Wait(ADUC_ProcessHandle handle, uint32_t timeoutMs);

/**
 * @brief Check if process is still running (non-blocking).
 */
bool ADUC_ProcessSupervisor_IsRunning(ADUC_ProcessHandle handle);

/**
 * @brief Get result (valid after Wait returns or IsRunning returns false).
 * @return 0 on success, negative on error.
 */
int ADUC_ProcessSupervisor_GetResult(ADUC_ProcessHandle handle, ADUC_ProcessResult* outResult);

/**
 * @brief Terminate process (SIGTERM, then SIGKILL after graceMs).
 * @return 0 on success, negative on error.
 */
int ADUC_ProcessSupervisor_Terminate(ADUC_ProcessHandle handle, uint32_t graceMs);

/**
 * @brief Kill process immediately (SIGKILL).
 * @return 0 on success, negative on error.
 */
int ADUC_ProcessSupervisor_Kill(ADUC_ProcessHandle handle);

/**
 * @brief Free captured output in a result struct.
 */
void ADUC_ProcessSupervisor_FreeResult(ADUC_ProcessResult* result);

/**
 * @brief Free handle and all associated resources.
 */
void ADUC_ProcessSupervisor_FreeHandle(ADUC_ProcessHandle handle);

/**
 * @brief Clean up orphaned processes from a previous agent run.
 *
 * Scans PID files in stateDir, checks if processes are still running, kills stale ones.
 * @return 0 on success, negative on error.
 */
int ADUC_ProcessSupervisor_CleanupOrphans(const char* stateDir);

/**
 * @brief Run a process synchronously (spawn + wait + get result in one call).
 * @return 0 on success, negative on error.
 */
int ADUC_ProcessSupervisor_Run(const ADUC_ProcessConfig* config, ADUC_ProcessResult* outResult);

#ifdef __cplusplus
}
#endif

#endif /* ADUC_PROCESS_SUPERVISOR_H */
