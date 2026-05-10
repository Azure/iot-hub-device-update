/**
 * @file log_strings.c
 * @brief Log string table mapping event IDs to format strings.
 *
 * Provides the centralized string table for all ADU Gen2 agent log events.
 * Used by the log writer for text-mode logging and by offline tools to
 * decode binary log files.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/log_strings.h"

#include <stddef.h>
#include <stdint.h>

typedef struct
{
    uint16_t eventId;
    const char* component;
    const char* format;
} ADUC_LogStringEntry;

/**
 * Master string table. Entries are sorted by event ID for binary search.
 */
static const ADUC_LogStringEntry s_logStrings[] = {
    // ── Agent Core (1xxx) ──
    { ADUC_EVT_AGENT_STARTING, "agent", "Agent starting (version=%s, device=%s)" },
    { ADUC_EVT_AGENT_STARTED, "agent", "Agent started successfully (pid=%d)" },
    { ADUC_EVT_AGENT_SHUTTING_DOWN, "agent", "Agent shutting down (reason=%s)" },
    { ADUC_EVT_AGENT_SHUTDOWN_COMPLETE, "agent", "Agent shutdown complete (uptime=%lu seconds)" },
    { ADUC_EVT_AGENT_CONFIG_LOADED, "agent", "Configuration loaded from %s" },
    { ADUC_EVT_AGENT_CONFIG_ERROR, "agent", "Configuration error: %s (file=%s, line=%d)" },
    { ADUC_EVT_AGENT_EXTENSION_LOADED, "agent", "Extension loaded: %s (version=%s)" },
    { ADUC_EVT_AGENT_EXTENSION_FAILED, "agent", "Extension load failed: %s (rc=0x%08x)" },
    { ADUC_EVT_AGENT_SCAN_DIR, "agent", "Scanning extension directory: %s" },
    { ADUC_EVT_AGENT_MODE_ONCE, "agent", "Running in single-deployment mode" },
    { ADUC_EVT_AGENT_MODE_DAEMON, "agent", "Running in daemon mode (poll_interval=%d seconds)" },
    { ADUC_EVT_AGENT_POLL_START, "agent", "Starting deployment poll cycle #%lu" },
    { ADUC_EVT_AGENT_POLL_NO_DEPLOYMENT, "agent", "Poll complete: no pending deployment" },
    { ADUC_EVT_AGENT_POLL_DEPLOYMENT, "agent", "Poll found deployment: %s (provider=%s, name=%s, version=%s)" },

    // ── Workflow Engine (2xxx) ──
    { ADUC_EVT_WF_EXECUTE_START, "workflow", "Workflow execution started: %s (steps=%d)" },
    { ADUC_EVT_WF_EXECUTE_COMPLETE, "workflow", "Workflow completed successfully: %s (duration=%lu ms)" },
    { ADUC_EVT_WF_EXECUTE_FAILED, "workflow", "Workflow failed: %s (rc=0x%08x, step=%s)" },
    { ADUC_EVT_WF_EXECUTE_CANCELLED, "workflow", "Workflow cancelled: %s (by=%s)" },
    { ADUC_EVT_WF_STEP_START, "workflow", "Step started: %s (index=%d, handler=%s)" },
    { ADUC_EVT_WF_STEP_COMPLETE, "workflow", "Step completed: %s (index=%d, duration=%lu ms)" },
    { ADUC_EVT_WF_STEP_FAILED, "workflow", "Step failed: %s (index=%d, rc=0x%08x, detail=%s)" },
    { ADUC_EVT_WF_STEP_SKIPPED, "workflow", "Step skipped: %s (index=%d, reason=%s)" },
    { ADUC_EVT_WF_HANDLER_NOT_FOUND, "workflow", "No handler found for step type: %s" },
    { ADUC_EVT_WF_CANCEL_REQUESTED, "workflow", "Cancel requested for workflow: %s (source=%s)" },
    { ADUC_EVT_WF_PROGRESS, "workflow", "Workflow progress: %s (%d%% complete, step %d/%d)" },

    // ── Extension Framework (3xxx) ──
    { ADUC_EVT_EXT_LOAD_START, "extension", "Loading extension: %s from %s" },
    { ADUC_EVT_EXT_LOAD_SUCCESS, "extension", "Extension loaded: %s (version=%s, capabilities=%s)" },
    { ADUC_EVT_EXT_LOAD_FAILED, "extension", "Extension load failed: %s (path=%s, error=%s)" },
    { ADUC_EVT_EXT_INIT_SUCCESS, "extension", "Extension initialized: %s" },
    { ADUC_EVT_EXT_INIT_FAILED, "extension", "Extension init failed: %s (rc=0x%08x)" },
    { ADUC_EVT_EXT_UNINIT, "extension", "Extension uninitialized: %s" },
    { ADUC_EVT_EXT_SCAN_DIR_START, "extension", "Scanning extension directory: %s" },
    { ADUC_EVT_EXT_SCAN_DIR_COMPLETE, "extension", "Extension scan complete: %s (%d extensions found)" },
    { ADUC_EVT_EXT_CAPABILITY_MATCH, "extension", "Capability matched: %s -> extension %s" },
    { ADUC_EVT_EXT_CAPABILITY_NOT_FOUND, "extension", "No extension found for capability: %s" },

    // ── Communication (4xxx) ──
    { ADUC_EVT_COMM_CONNECT, "comm", "Connected to service endpoint: %s (protocol=%s)" },
    { ADUC_EVT_COMM_CONNECT_FAILED, "comm", "Connection failed: %s (rc=0x%08x, attempt=%d/%d)" },
    { ADUC_EVT_COMM_DISCONNECT, "comm", "Disconnected from service: %s (reason=%s)" },
    { ADUC_EVT_COMM_POLL, "comm", "Polling for deployment (endpoint=%s)" },
    { ADUC_EVT_COMM_POLL_RESULT, "comm", "Poll result: %s (status=%d, latency=%lu ms)" },
    { ADUC_EVT_COMM_REPORT_STATE, "comm", "Reporting device state: %s (workflow=%s)" },
    { ADUC_EVT_COMM_REPORT_RESULT, "comm", "State report accepted (workflow=%s, code=%d)" },
    { ADUC_EVT_COMM_REPORT_FAILED, "comm", "State report failed (workflow=%s, rc=0x%08x, retry=%d)" },

    // ── Download (5xxx) ──
    { ADUC_EVT_DL_START, "download", "Download started: %s (size=%lu bytes, dest=%s)" },
    { ADUC_EVT_DL_PROGRESS, "download", "Download progress: %s (%lu/%lu bytes, %d%%)" },
    { ADUC_EVT_DL_COMPLETE, "download", "Download complete: %s (size=%lu bytes, duration=%lu ms)" },
    { ADUC_EVT_DL_FAILED, "download", "Download failed: %s (rc=0x%08x, http_status=%d)" },
    { ADUC_EVT_DL_RETRY, "download", "Download retry: %s (attempt=%d/%d, backoff=%lu ms)" },
    { ADUC_EVT_DL_HASH_VERIFY, "download", "Hash verification passed: %s (algorithm=%s)" },
    { ADUC_EVT_DL_HASH_MISMATCH, "download", "Hash mismatch: %s (expected=%s, actual=%s)" },

    // ── Security (6xxx) ──
    { ADUC_EVT_SEC_CERT_LOAD, "security", "Certificate loaded: %s (subject=%s, expires=%s)" },
    { ADUC_EVT_SEC_CERT_EXPIRED, "security", "Certificate expired: %s (expired=%s)" },
    { ADUC_EVT_SEC_SIG_VERIFY, "security", "Signature verification passed: %s (signer=%s)" },
    { ADUC_EVT_SEC_SIG_FAILED, "security", "Signature verification failed: %s (error=%s)" },
    { ADUC_EVT_SEC_KEY_LOAD, "security", "Signing key loaded: %s (type=%s, bits=%d)" },
    { ADUC_EVT_SEC_KEY_FAILED, "security", "Signing key load failed: %s (error=%s)" },

    // ── DAG Engine (7xxx) ──
    { ADUC_EVT_DAG_CREATE, "dag", "DAG created: %s (%d nodes, %d edges)" },
    { ADUC_EVT_DAG_CYCLE_DETECTED, "dag", "Cycle detected in DAG: %s (nodes involved: %s)" },
    { ADUC_EVT_DAG_NODE_READY, "dag", "Node ready for execution: %s (dependencies satisfied)" },
    { ADUC_EVT_DAG_NODE_DONE, "dag", "Node completed: %s (duration=%lu ms)" },
    { ADUC_EVT_DAG_NODE_FAILED, "dag", "Node failed: %s (rc=0x%08x)" },
    { ADUC_EVT_DAG_NODE_SKIPPED, "dag", "Node skipped: %s (reason=%s)" },
    { ADUC_EVT_DAG_COMPLETE, "dag", "DAG execution complete: %s (nodes_ok=%d, nodes_failed=%d)" },
    { ADUC_EVT_DAG_DEPENDENCY_RESOLVED, "dag", "Dependency resolved: %s -> %s" },
    { ADUC_EVT_DAG_SKIP_ON_FAILED, "dag", "Skipping node %s due to failed dependency: %s" },
    { ADUC_EVT_DAG_RUN_ON_FAILED, "dag", "Running node %s despite failed dependency (runOnFailed=true)" },

    // ── Script Handler (8xxx) ──
    { ADUC_EVT_SCRIPT_EVALUATE, "script", "Evaluating script handler for step: %s (type=%s)" },
    { ADUC_EVT_SCRIPT_EXECUTE_START, "script", "Script execution started: %s (cmd=%s, args=%s)" },
    { ADUC_EVT_SCRIPT_EXECUTE_COMPLETE, "script", "Script completed: %s (exit_code=%d, duration=%lu ms)" },
    { ADUC_EVT_SCRIPT_EXECUTE_TIMEOUT, "script", "Script timed out: %s (timeout=%d seconds, pid=%d)" },
    { ADUC_EVT_SCRIPT_EXECUTE_FAILED, "script", "Script failed: %s (exit_code=%d, stderr=%s)" },
    { ADUC_EVT_SCRIPT_ROLLBACK, "script", "Script rollback started: %s (original_step=%s)" },
    { ADUC_EVT_SCRIPT_INSTALLED_CHECK, "script", "Script installed check: %s (result=%s)" },
    { ADUC_EVT_SCRIPT_RESULT_FILE, "script", "Script result file: %s (path=%s, rc=0x%08x)" },

    // ── APT Handler (9xxx) ──
    { ADUC_EVT_APT_EVALUATE, "apt", "Evaluating APT handler for step: %s (packages=%s)" },
    { ADUC_EVT_APT_UPDATE_INDEX, "apt", "Updating APT package index (sources=%s)" },
    { ADUC_EVT_APT_INSTALL_START, "apt", "APT install started: %s (packages=%s)" },
    { ADUC_EVT_APT_INSTALL_COMPLETE, "apt", "APT install complete: %s (%d packages installed)" },
    { ADUC_EVT_APT_INSTALL_FAILED, "apt", "APT install failed: %s (package=%s, error=%s)" },
    { ADUC_EVT_APT_VALIDATE, "apt", "APT validation: %s (package=%s, expected=%s, actual=%s)" },
    { ADUC_EVT_APT_ROLLBACK, "apt", "APT rollback started: %s (packages=%s)" },

    // ── SWUpdate Handler (Axxx) ──
    { ADUC_EVT_SWU_EVALUATE, "swupdate", "Evaluating SWUpdate handler for step: %s" },
    { ADUC_EVT_SWU_EXECUTE_START, "swupdate", "SWUpdate execution started: %s (image=%s)" },
    { ADUC_EVT_SWU_PROGRESS, "swupdate", "SWUpdate progress: %s (%d%% complete)" },
    { ADUC_EVT_SWU_EXECUTE_COMPLETE, "swupdate", "SWUpdate complete: %s (duration=%lu ms)" },
    { ADUC_EVT_SWU_EXECUTE_FAILED, "swupdate", "SWUpdate failed: %s (rc=0x%08x, detail=%s)" },
    { ADUC_EVT_SWU_VERIFY, "swupdate", "SWUpdate verification: %s (slot=%s, status=%s)" },
    { ADUC_EVT_SWU_REBOOT_SIGNAL, "swupdate", "SWUpdate reboot signal sent (delay=%d seconds)" },
};

static const size_t s_logStringsCount = sizeof(s_logStrings) / sizeof(s_logStrings[0]);

const char* ADUC_LogString_GetFormat(uint16_t eventId)
{
    for (size_t i = 0; i < s_logStringsCount; i++)
    {
        if (s_logStrings[i].eventId == eventId)
        {
            return s_logStrings[i].format;
        }
    }
    return NULL;
}

const char* ADUC_LogString_GetComponent(uint16_t eventId)
{
    for (size_t i = 0; i < s_logStringsCount; i++)
    {
        if (s_logStrings[i].eventId == eventId)
        {
            return s_logStrings[i].component;
        }
    }
    return NULL;
}

size_t ADUC_LogString_GetCount(void)
{
    return s_logStringsCount;
}
