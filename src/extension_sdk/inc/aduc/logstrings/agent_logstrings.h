/**
 * @file agent_logstrings.h
 * @brief Log format strings for agent lifecycle events.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_AGENT_LOGSTRINGS_H
#define ADUC_AGENT_LOGSTRINGS_H

// Agent lifecycle
#define ADUC_LOG_AGENT_STARTING          "ADU agent starting, version=%s, profile=%d"
#define ADUC_LOG_AGENT_CONFIG_LOADED     "Configuration loaded from %s, %d extensions configured"
#define ADUC_LOG_AGENT_EXT_LOADED        "Extension loaded: type=%d, name=%s, caps=%s"
#define ADUC_LOG_AGENT_EXT_LOAD_FAIL     "Failed to load extension: path=%s, error=%s"
#define ADUC_LOG_AGENT_POLL_START        "Poll cycle starting, interval=%ds"
#define ADUC_LOG_AGENT_POLL_NO_UPDATE    "No update available"
#define ADUC_LOG_AGENT_POLL_UPDATE       "Update available: workflowId=%s"
#define ADUC_LOG_AGENT_SHUTDOWN          "Agent shutting down, reason=%s"
#define ADUC_LOG_AGENT_DOWNLOAD_START    "Download starting: fileId=%s, url=%s"
#define ADUC_LOG_AGENT_DOWNLOAD_DONE     "Download complete: fileId=%s, size=%ld bytes"
#define ADUC_LOG_AGENT_DOWNLOAD_FAIL     "Download failed: fileId=%s, rc=0x%08X"
#define ADUC_LOG_AGENT_REPORT_SUCCESS    "Reporting success: workflowId=%s"
#define ADUC_LOG_AGENT_REPORT_FAILURE    "Reporting failure: workflowId=%s, outcome=%s, origin=%s"

#endif // ADUC_AGENT_LOGSTRINGS_H
