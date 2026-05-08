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
 * @brief Authoritative terminal outcome of a deployment (v2 protocol).
 */
typedef enum ADUC_Outcome
{
    ADUC_Outcome_Succeeded = 0,
    ADUC_Outcome_Failed = 1,
    ADUC_Outcome_Cancelled = 2,
    ADUC_Outcome_Skipped = 3
} ADUC_Outcome;

/**
 * @brief Convert ADUC_Outcome to string.
 */
static inline const char* ADUC_Outcome_ToString(ADUC_Outcome o)
{
    switch (o)
    {
        case ADUC_Outcome_Succeeded: return "SUCCEEDED";
        case ADUC_Outcome_Failed:    return "FAILED";
        case ADUC_Outcome_Cancelled: return "CANCELLED";
        case ADUC_Outcome_Skipped:   return "SKIPPED";
        default:                     return "FAILED";
    }
}

/**
 * @brief Advisory hint for failure source (v2 protocol).
 */
typedef enum ADUC_Origin
{
    ADUC_Origin_AduService = 0,
    ADUC_Origin_AduResource = 1,
    ADUC_Origin_AgentCore = 2,
    ADUC_Origin_AgentExtension = 3,
    ADUC_Origin_Device = 4
} ADUC_Origin;

/**
 * @brief Convert ADUC_Origin to string.
 */
static inline const char* ADUC_Origin_ToString(ADUC_Origin o)
{
    switch (o)
    {
        case ADUC_Origin_AduService:     return "ADU_SERVICE";
        case ADUC_Origin_AduResource:    return "ADU_RESOURCE";
        case ADUC_Origin_AgentCore:      return "AGENT_CORE";
        case ADUC_Origin_AgentExtension: return "AGENT_EXTENSION";
        case ADUC_Origin_Device:         return "DEVICE";
        default:                         return "AGENT_CORE";
    }
}

#ifdef __cplusplus
}
#endif

#endif // ADUC_EXTENSION_TYPES_H
