/**
 * @file log_strings.h
 * @brief Log Event ID Registry for ADU Gen2 Agent.
 *
 * Extracts all format strings from Gen2 components into a centralized
 * registry. Enables binary logging (store event IDs only, decode with
 * string table offline), localization (translate format strings without
 * rebuilding), and log analysis (search by event ID, not by text).
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_LOG_STRINGS_H
#define ADUC_LOG_STRINGS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Log Event ID Registry
 *
 * Format: ADUC_EVT_{COMPONENT}_{DESCRIPTION}
 * Convention:
 * - Event IDs are uint16_t, unique per component
 * - Component prefix is 4-digit: 1xxx=Agent, 2xxx=Workflow, 3xxx=Extension,
 *   4xxx=Comm, 5xxx=Download, 6xxx=Security, 7xxx=DAG, 8xxx=ScriptHandler,
 *   9xxx=AptHandler, Axxx=SwuHandler
 */

// ══════════ Agent Core (1xxx) ══════════
#define ADUC_EVT_AGENT_STARTING             1001
#define ADUC_EVT_AGENT_STARTED              1002
#define ADUC_EVT_AGENT_SHUTTING_DOWN        1003
#define ADUC_EVT_AGENT_SHUTDOWN_COMPLETE    1004
#define ADUC_EVT_AGENT_CONFIG_LOADED        1010
#define ADUC_EVT_AGENT_CONFIG_ERROR         1011
#define ADUC_EVT_AGENT_EXTENSION_LOADED     1020
#define ADUC_EVT_AGENT_EXTENSION_FAILED     1021
#define ADUC_EVT_AGENT_SCAN_DIR             1022
#define ADUC_EVT_AGENT_MODE_ONCE            1030
#define ADUC_EVT_AGENT_MODE_DAEMON          1031
#define ADUC_EVT_AGENT_POLL_START           1040
#define ADUC_EVT_AGENT_POLL_NO_DEPLOYMENT   1041
#define ADUC_EVT_AGENT_POLL_DEPLOYMENT      1042

// ══════════ Workflow Engine (2xxx) ══════════
#define ADUC_EVT_WF_EXECUTE_START           2001
#define ADUC_EVT_WF_EXECUTE_COMPLETE        2002
#define ADUC_EVT_WF_EXECUTE_FAILED          2003
#define ADUC_EVT_WF_EXECUTE_CANCELLED       2004
#define ADUC_EVT_WF_STEP_START              2010
#define ADUC_EVT_WF_STEP_COMPLETE           2011
#define ADUC_EVT_WF_STEP_FAILED             2012
#define ADUC_EVT_WF_STEP_SKIPPED            2013
#define ADUC_EVT_WF_HANDLER_NOT_FOUND       2020
#define ADUC_EVT_WF_CANCEL_REQUESTED        2030
#define ADUC_EVT_WF_PROGRESS                2040

// ══════════ Extension Framework (3xxx) ══════════
#define ADUC_EVT_EXT_LOAD_START             3001
#define ADUC_EVT_EXT_LOAD_SUCCESS           3002
#define ADUC_EVT_EXT_LOAD_FAILED            3003
#define ADUC_EVT_EXT_INIT_SUCCESS           3010
#define ADUC_EVT_EXT_INIT_FAILED            3011
#define ADUC_EVT_EXT_UNINIT                 3012
#define ADUC_EVT_EXT_SCAN_DIR_START         3020
#define ADUC_EVT_EXT_SCAN_DIR_COMPLETE      3021
#define ADUC_EVT_EXT_CAPABILITY_MATCH       3030
#define ADUC_EVT_EXT_CAPABILITY_NOT_FOUND   3031

// ══════════ Communication (4xxx) ══════════
#define ADUC_EVT_COMM_CONNECT               4001
#define ADUC_EVT_COMM_CONNECT_FAILED        4002
#define ADUC_EVT_COMM_DISCONNECT            4003
#define ADUC_EVT_COMM_POLL                  4010
#define ADUC_EVT_COMM_POLL_RESULT           4011
#define ADUC_EVT_COMM_REPORT_STATE          4020
#define ADUC_EVT_COMM_REPORT_RESULT         4021
#define ADUC_EVT_COMM_REPORT_FAILED         4022

// ══════════ Download (5xxx) ══════════
#define ADUC_EVT_DL_START                   5001
#define ADUC_EVT_DL_PROGRESS                5002
#define ADUC_EVT_DL_COMPLETE                5003
#define ADUC_EVT_DL_FAILED                  5004
#define ADUC_EVT_DL_RETRY                   5010
#define ADUC_EVT_DL_HASH_VERIFY             5020
#define ADUC_EVT_DL_HASH_MISMATCH           5021

// ══════════ Security (6xxx) ══════════
#define ADUC_EVT_SEC_CERT_LOAD              6001
#define ADUC_EVT_SEC_CERT_EXPIRED           6002
#define ADUC_EVT_SEC_SIG_VERIFY             6010
#define ADUC_EVT_SEC_SIG_FAILED             6011
#define ADUC_EVT_SEC_KEY_LOAD               6020
#define ADUC_EVT_SEC_KEY_FAILED             6021

// ══════════ DAG Engine (7xxx) ══════════
#define ADUC_EVT_DAG_CREATE                 7001
#define ADUC_EVT_DAG_CYCLE_DETECTED         7002
#define ADUC_EVT_DAG_NODE_READY             7010
#define ADUC_EVT_DAG_NODE_DONE              7011
#define ADUC_EVT_DAG_NODE_FAILED            7012
#define ADUC_EVT_DAG_NODE_SKIPPED           7013
#define ADUC_EVT_DAG_COMPLETE               7020
#define ADUC_EVT_DAG_DEPENDENCY_RESOLVED    7030
#define ADUC_EVT_DAG_SKIP_ON_FAILED         7031
#define ADUC_EVT_DAG_RUN_ON_FAILED          7032

// ══════════ Script Handler (8xxx) ══════════
#define ADUC_EVT_SCRIPT_EVALUATE            8001
#define ADUC_EVT_SCRIPT_EXECUTE_START       8010
#define ADUC_EVT_SCRIPT_EXECUTE_COMPLETE    8011
#define ADUC_EVT_SCRIPT_EXECUTE_TIMEOUT     8012
#define ADUC_EVT_SCRIPT_EXECUTE_FAILED      8013
#define ADUC_EVT_SCRIPT_ROLLBACK            8020
#define ADUC_EVT_SCRIPT_INSTALLED_CHECK     8030
#define ADUC_EVT_SCRIPT_RESULT_FILE         8040

// ══════════ APT Handler (9xxx) ══════════
#define ADUC_EVT_APT_EVALUATE               9001
#define ADUC_EVT_APT_UPDATE_INDEX           9010
#define ADUC_EVT_APT_INSTALL_START          9020
#define ADUC_EVT_APT_INSTALL_COMPLETE       9021
#define ADUC_EVT_APT_INSTALL_FAILED         9022
#define ADUC_EVT_APT_VALIDATE               9030
#define ADUC_EVT_APT_ROLLBACK               9040

// ══════════ SWUpdate Handler (Axxx = 0xA000) ══════════
#define ADUC_EVT_SWU_EVALUATE               0xA001
#define ADUC_EVT_SWU_EXECUTE_START          0xA010
#define ADUC_EVT_SWU_PROGRESS               0xA011
#define ADUC_EVT_SWU_EXECUTE_COMPLETE       0xA012
#define ADUC_EVT_SWU_EXECUTE_FAILED         0xA013
#define ADUC_EVT_SWU_VERIFY                 0xA020
#define ADUC_EVT_SWU_REBOOT_SIGNAL          0xA030

/**
 * @brief Look up the format string for a given event ID.
 * @param eventId The event ID to look up.
 * @return The format string, or NULL if the event ID is unknown.
 */
const char* ADUC_LogString_GetFormat(uint16_t eventId);

/**
 * @brief Look up the component name for a given event ID.
 * @param eventId The event ID to look up.
 * @return The component name, or NULL if the event ID is unknown.
 */
const char* ADUC_LogString_GetComponent(uint16_t eventId);

/**
 * @brief Get the total number of registered log string entries.
 * @return The count of entries in the string table.
 */
size_t ADUC_LogString_GetCount(void);

#ifdef __cplusplus
}
#endif
#endif // ADUC_LOG_STRINGS_H
