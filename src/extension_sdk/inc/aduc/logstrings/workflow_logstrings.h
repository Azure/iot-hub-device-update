/**
 * @file workflow_logstrings.h
 * @brief Log format strings for workflow and orchestration events.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_WORKFLOW_LOGSTRINGS_H
#define ADUC_WORKFLOW_LOGSTRINGS_H

#define ADUC_LOG_WF_START               "Workflow starting: workflowId=%s, steps=%d"
#define ADUC_LOG_WF_STEP_BEGIN          "Step %d/%d starting: handler=%s, phase=%s"
#define ADUC_LOG_WF_STEP_DONE           "Step %d/%d completed: outcome=%s, duration=%lums"
#define ADUC_LOG_WF_STEP_FAIL           "Step %d/%d failed: rc=0x%08X, erc=%s"
#define ADUC_LOG_WF_STEP_SKIP           "Step %d/%d skipped: reason=%s"
#define ADUC_LOG_WF_DAG_ORDER           "DAG execution order: %s"
#define ADUC_LOG_WF_DAG_CYCLE           "DAG cycle detected involving step %d"
#define ADUC_LOG_WF_COMPLETE            "Workflow complete: workflowId=%s, outcome=%s, duration=%lums"
#define ADUC_LOG_WF_MANIFEST_VERIFY     "Manifest signature verification: %s"
#define ADUC_LOG_WF_HASH_VERIFY         "File hash verification: fileId=%s, algorithm=%s, result=%s"
#define ADUC_LOG_WF_REBOOT_NEEDED       "Reboot required after step %d"
#define ADUC_LOG_WF_RESUME              "Resuming workflow from step %d after restart"

#endif // ADUC_WORKFLOW_LOGSTRINGS_H
