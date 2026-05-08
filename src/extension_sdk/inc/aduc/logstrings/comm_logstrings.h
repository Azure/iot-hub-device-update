/**
 * @file comm_logstrings.h
 * @brief Log format strings for communication provider events.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_COMM_LOGSTRINGS_H
#define ADUC_COMM_LOGSTRINGS_H

#define ADUC_LOG_COMM_CONNECT           "Connecting to service: endpoint=%s"
#define ADUC_LOG_COMM_CONNECTED         "Connected to service successfully"
#define ADUC_LOG_COMM_DISCONNECT        "Disconnected from service: reason=%s"
#define ADUC_LOG_COMM_POLL              "Polling for updates..."
#define ADUC_LOG_COMM_SYNC_CONFIG       "Syncing configuration: agentInfoETag=%s"
#define ADUC_LOG_COMM_SYNC_DONE         "Configuration synced: serviceConfigETag=%s"
#define ADUC_LOG_COMM_REPORT            "Reporting status: workflowId=%s, outcome=%s"
#define ADUC_LOG_COMM_REPORT_DONE       "Status reported successfully"
#define ADUC_LOG_COMM_REPORT_FAIL       "Status report failed: httpStatus=%d, error=%s"
#define ADUC_LOG_COMM_THROTTLED         "Request throttled: retryAfter=%ds"
#define ADUC_LOG_COMM_TLS_ERROR         "TLS error: %s"
#define ADUC_LOG_COMM_SIM_MANIFEST      "Simulator: reading manifest from %s"
#define ADUC_LOG_COMM_SIM_RESULT        "Simulator: writing result to %s"

#endif // ADUC_COMM_LOGSTRINGS_H
