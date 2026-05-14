/**
 * @file extension_types.h
 * @brief Core type definitions for the ADU Gen2 unified extension model.
 *
 * This header defines the types shared across the extension SDK:
 * extension types, result codes, log levels, and common structures.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_EXTENSION_TYPES_H
#define ADUC_EXTENSION_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Extension types supported by the ADU agent.
 */
typedef enum ADUC_ExtensionType
{
    ADUC_EXT_TYPE_COMMUNICATION = 1,
    ADUC_EXT_TYPE_STEP_HANDLER = 2,
    ADUC_EXT_TYPE_CONTENT_PROCESSOR = 3,
    ADUC_EXT_TYPE_DOWNLOADER = 4,
    ADUC_EXT_TYPE_COMPONENT_ENUMERATOR = 5,
    ADUC_EXT_TYPE_AUTHENTICATOR = 6,
} ADUC_ExtensionType;

/**
 * @brief Structured result type.
 *
 * Layout: [facility:8][category:8][specific:16]
 * Facility 0 with code 0 = success.
 */
typedef struct ADUC_Result2
{
    uint32_t code;
} ADUC_Result2;

#define ADUC_RESULT2_SUCCESS ((ADUC_Result2){ .code = 0 })
#define ADUC_RESULT2_IS_SUCCESS(r) ((r).code == 0)
#define ADUC_RESULT2_IS_FAILURE(r) ((r).code != 0)

#define ADUC_RESULT2_MAKE(facility, category, specific) \
    ((ADUC_Result2){ .code = ((uint32_t)(facility) << 24) | ((uint32_t)(category) << 16) | (uint32_t)(specific) })

// Facilities
#define ADUC_FACILITY_AGENT     0x01
#define ADUC_FACILITY_DOWNLOAD  0x02
#define ADUC_FACILITY_WORKFLOW  0x03
#define ADUC_FACILITY_EXTENSION 0x04
#define ADUC_FACILITY_COMM      0x05
#define ADUC_FACILITY_SECURITY  0x06

// Categories
#define ADUC_CATEGORY_CONFIG    0x01
#define ADUC_CATEGORY_AUTH      0x02
#define ADUC_CATEGORY_IO        0x03
#define ADUC_CATEGORY_PROTOCOL  0x04
#define ADUC_CATEGORY_RESOURCE  0x05
#define ADUC_CATEGORY_STATE     0x06

/**
 * @brief Log severity levels.
 */
typedef enum ADUC_LogLevel
{
    ADUC_LOG_FATAL = 1,
    ADUC_LOG_ERROR = 2,
    ADUC_LOG_WARN = 3,
    ADUC_LOG_INFO = 4,
    ADUC_LOG_DEBUG = 5,
    ADUC_LOG_TRACE = 6,
} ADUC_LogLevel;

/**
 * @brief Key-value property map (flat, string-only values).
 */
typedef struct ADUC_PropertyEntry
{
    const char* key;
    const char* value;
} ADUC_PropertyEntry;

typedef struct ADUC_PropertyMap
{
    ADUC_PropertyEntry* entries;
    size_t count;
} ADUC_PropertyMap;

/**
 * @brief Semantic version.
 */
typedef struct ADUC_Version
{
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
} ADUC_Version;

/**
 * @brief Authoritative terminal outcome of a deployment (v4 protocol).
 */
typedef enum ADUC_Outcome
{
    ADUC_Outcome_Succeeded = 0,
    ADUC_Outcome_Failed = 1,
    ADUC_Outcome_Canceled = 2,
    ADUC_Outcome_Skipped = 3
} ADUC_Outcome;

/* Backward compat alias for Gen1 code that uses British spelling */
#define ADUC_Outcome_Cancelled ADUC_Outcome_Canceled

/**
 * @brief Convert ADUC_Outcome to v3 wire-format string.
 */
static inline const char* ADUC_Outcome_ToString(ADUC_Outcome o)
{
    switch (o)
    {
        case ADUC_Outcome_Succeeded: return "SUCCEEDED";
        case ADUC_Outcome_Failed:    return "FAILED";
        case ADUC_Outcome_Canceled:  return "CANCELED";
        case ADUC_Outcome_Skipped:   return "SKIPPED";
        default:                     return "FAILED";
    }
}

/**
 * @brief Advisory hint identifying which subsystem produced a failure (v4 protocol).
 *
 * Required on every reportStatus. Advisory only — the service MUST NOT use
 * this for state decisions. On success/cancel/skip, set to NOT_APPLICABLE.
 * v4: FAILED + NOT_APPLICABLE is prohibited — use OTHER for unclassifiable failures.
 */
typedef enum ADUC_FailureOrigin
{
    ADUC_FailureOrigin_NotApplicable = 0,
    ADUC_FailureOrigin_AduCloudService = 1,
    ADUC_FailureOrigin_AduManagedResource = 2,
    ADUC_FailureOrigin_AgentCore = 3,
    ADUC_FailureOrigin_AgentExtension = 4,
    ADUC_FailureOrigin_AgentDependency = 5,
    ADUC_FailureOrigin_Device = 6,
    ADUC_FailureOrigin_Other = 7
} ADUC_FailureOrigin;

/* Backward compat aliases for code using the old ADUC_Origin names */
typedef ADUC_FailureOrigin ADUC_Origin;
#define ADUC_Origin_AduService      ADUC_FailureOrigin_AduCloudService
#define ADUC_Origin_AduResource     ADUC_FailureOrigin_AduManagedResource
#define ADUC_Origin_AgentCore       ADUC_FailureOrigin_AgentCore
#define ADUC_Origin_AgentExtension  ADUC_FailureOrigin_AgentExtension
#define ADUC_Origin_Device          ADUC_FailureOrigin_Device

/**
 * @brief Convert ADUC_FailureOrigin to v3 wire-format string.
 */
static inline const char* ADUC_FailureOrigin_ToString(ADUC_FailureOrigin o)
{
    switch (o)
    {
        case ADUC_FailureOrigin_NotApplicable:      return "NOT_APPLICABLE";
        case ADUC_FailureOrigin_AduCloudService:    return "ADU_CLOUD_SERVICE";
        case ADUC_FailureOrigin_AduManagedResource: return "ADU_MANAGED_RESOURCE";
        case ADUC_FailureOrigin_AgentCore:          return "AGENT_CORE";
        case ADUC_FailureOrigin_AgentExtension:     return "AGENT_EXTENSION";
        case ADUC_FailureOrigin_AgentDependency:    return "AGENT_DEPENDENCY";
        case ADUC_FailureOrigin_Device:             return "DEVICE";
        case ADUC_FailureOrigin_Other:              return "OTHER";
        default:                                    return "OTHER";
    }
}

/* Backward compat alias */
#define ADUC_Origin_ToString ADUC_FailureOrigin_ToString

/**
 * @brief Agent protocol states (v4 protocol §7.1).
 * The agent maintains exactly one of these states at any time.
 */
typedef enum ADUC_AgentState_v3
{
    ADUC_AGENT_STATE_IDLE = 0,
    ADUC_AGENT_STATE_CONTENT_DOWNLOADING = 1,
    ADUC_AGENT_STATE_INSTALLING = 2,
    ADUC_AGENT_STATE_REBOOTING = 3,
    ADUC_AGENT_STATE_REPORTING = 4
} ADUC_AgentState_v3;

static inline const char* ADUC_AgentState_ToString(ADUC_AgentState_v3 s)
{
    switch (s)
    {
        case ADUC_AGENT_STATE_IDLE:                 return "IDLE";
        case ADUC_AGENT_STATE_CONTENT_DOWNLOADING:  return "CONTENT_DOWNLOADING";
        case ADUC_AGENT_STATE_INSTALLING:            return "INSTALLING";
        case ADUC_AGENT_STATE_REBOOTING:             return "REBOOTING";
        case ADUC_AGENT_STATE_REPORTING:             return "REPORTING";
        default:                                    return "IDLE";
    }
}

#ifdef __cplusplus
}
#endif

#endif // ADUC_EXTENSION_TYPES_H
