/**
 * @file result.h
 * @brief Azure Device Update Core SDK Result Types
 *
 * Common result codes and structures used across all ADU extensions.
 * This provides a standardized way to report success, errors, and detailed
 * information about operation outcomes.
 *
 * @copyright Copyright (C) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See LICENSE file in the project root for license information.
 */

#ifndef ADUC_RESULT_H
#define ADUC_RESULT_H

#include <stdbool.h>
#include <stdint.h>
#include "aduc/exports.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Extended result code type.
 * Provides detailed error information beyond simple success/failure.
 */
typedef int32_t ADUC_Result_t;

/**
 * @brief Standard ADUC result codes.
 */
typedef enum tagADUC_Result
{
    ADUC_Result_Failure = 0,                    /**< General failure */
    ADUC_Result_Success = 1,                    /**< Operation succeeded */

    // General error codes (100-199)
    ADUC_Result_Failure_Cancelled = 100,       /**< Operation was cancelled */
    ADUC_Result_Failure_InvalidArgument = 101, /**< Invalid argument provided */
    ADUC_Result_Failure_OutOfMemory = 102,     /**< Out of memory */
    ADUC_Result_Failure_NotSupported = 103,    /**< Operation not supported */
    ADUC_Result_Failure_NotImplemented = 104,  /**< Feature not implemented */
    ADUC_Result_Failure_Timeout = 105,         /**< Operation timed out */

    // File system error codes (200-299)
    ADUC_Result_Failure_FileNotFound = 200,    /**< File not found */
    ADUC_Result_Failure_FileAccess = 201,      /**< File access denied */
    ADUC_Result_Failure_DiskFull = 202,        /**< Insufficient disk space */
    ADUC_Result_Failure_FileCorrupt = 203,     /**< File is corrupted */
    ADUC_Result_Failure_FileExists = 204,      /**< File already exists */

    // Network error codes (300-399)
    ADUC_Result_Failure_NetworkError = 300,    /**< Network communication error */
    ADUC_Result_Failure_ConnectionTimeout = 301, /**< Network connection timeout */
    ADUC_Result_Failure_HostNotFound = 302,    /**< Host not found */
    ADUC_Result_Failure_Unauthorized = 303,    /**< Authentication failed */
    ADUC_Result_Failure_Forbidden = 304,       /**< Access forbidden */

    // Content/Package error codes (400-499)
    ADUC_Result_Failure_ContentNotFound = 400, /**< Update content not found */
    ADUC_Result_Failure_InvalidManifest = 401, /**< Invalid update manifest */
    ADUC_Result_Failure_InvalidContent = 402,  /**< Invalid update content */
    ADUC_Result_Failure_VerificationFailed = 403, /**< Content verification failed */
    ADUC_Result_Failure_HashMismatch = 404,    /**< Content hash mismatch */
    ADUC_Result_Failure_SignatureInvalid = 405, /**< Digital signature invalid */

    // Installation error codes (500-599)
    ADUC_Result_Failure_InstallFailed = 500,   /**< Installation failed */
    ADUC_Result_Failure_UninstallFailed = 501, /**< Uninstallation failed */
    ADUC_Result_Failure_ApplyFailed = 502,     /**< Apply operation failed */
    ADUC_Result_Failure_RollbackFailed = 503,  /**< Rollback operation failed */
    ADUC_Result_Failure_RestartRequired = 504, /**< Restart required but not allowed */
    ADUC_Result_Failure_IncompatibleVersion = 505, /**< Incompatible version */

    // Configuration error codes (600-699)
    ADUC_Result_Failure_ConfigError = 600,     /**< Configuration error */
    ADUC_Result_Failure_MissingConfig = 601,   /**< Required configuration missing */
    ADUC_Result_Failure_InvalidConfig = 602,   /**< Invalid configuration */

    // System error codes (700-799)
    ADUC_Result_Failure_SystemError = 700,     /**< System-level error */
    ADUC_Result_Failure_ServiceUnavailable = 701, /**< Required service unavailable */
    ADUC_Result_Failure_PermissionDenied = 702, /**< Insufficient permissions */
    ADUC_Result_Failure_ResourceBusy = 703,    /**< Resource is busy */

    // Extension-specific codes (800-999)
    ADUC_Result_Failure_ExtensionError = 800,  /**< Extension-specific error */
    ADUC_Result_Failure_HandlerNotFound = 801, /**< Required handler not found */
    ADUC_Result_Failure_HandlerLoadFailed = 802, /**< Handler failed to load */
    ADUC_Result_Failure_InterfaceError = 803,  /**< Extension interface error */

    // Reserved for future use (1000+)
    ADUC_Result_Failure_Reserved = 1000
} ADUC_Result;

/**
 * @brief Result details structure.
 * Provides additional context and information about operation results.
 */
typedef struct tagADUC_ResultDetails
{
    ADUC_Result_t resultCode;         /**< Primary result code */
    ADUC_Result_t extendedResultCode; /**< Extended/detailed result code */
    const char* resultDetails;        /**< Human-readable result description */
    const char* stepId;              /**< Step identifier for multi-step operations */
} ADUC_ResultDetails;

/**
 * @brief Initialize result details structure.
 *
 * @param details Pointer to result details structure to initialize
 * @param resultCode Primary result code
 * @param extendedResultCode Extended result code (0 if not applicable)
 * @param resultDetails Human-readable description (can be NULL)
 * @param stepId Step identifier (can be NULL)
 */
ADUC_SDK_EXPORT void ADUC_Result_Init(
    ADUC_ResultDetails* details,
    ADUC_Result_t resultCode,
    ADUC_Result_t extendedResultCode,
    const char* resultDetails,
    const char* stepId);

/**
 * @brief Create a success result.
 *
 * @param details Pointer to result details structure to populate
 * @param description Optional success description
 */
ADUC_SDK_EXPORT void ADUC_Result_SetSuccess(ADUC_ResultDetails* details, const char* description);

/**
 * @brief Create a failure result.
 *
 * @param details Pointer to result details structure to populate
 * @param resultCode Failure result code
 * @param description Error description
 */
ADUC_SDK_EXPORT void ADUC_Result_SetFailure(
    ADUC_ResultDetails* details,
    ADUC_Result_t resultCode,
    const char* description);

/**
 * @brief Check if result indicates success.
 *
 * @param details Result details to check
 * @return true if result indicates success, false otherwise
 */
ADUC_SDK_EXPORT bool ADUC_Result_IsSuccess(const ADUC_ResultDetails* details);

/**
 * @brief Check if result indicates failure.
 *
 * @param details Result details to check
 * @return true if result indicates failure, false otherwise
 */
ADUC_SDK_EXPORT bool ADUC_Result_IsFailure(const ADUC_ResultDetails* details);

/**
 * @brief Get result code as string.
 *
 * @param resultCode Result code to convert
 * @return String representation of result code
 */
ADUC_SDK_EXPORT const char* ADUC_Result_ToString(ADUC_Result_t resultCode);

/**
 * @brief Copy result details.
 *
 * @param dest Destination result details
 * @param src Source result details
 */
ADUC_SDK_EXPORT void ADUC_Result_Copy(ADUC_ResultDetails* dest, const ADUC_ResultDetails* src);

/**
 * @brief Free any allocated memory in result details.
 *
 * @param details Result details to clean up
 */
ADUC_SDK_EXPORT void ADUC_Result_Free(ADUC_ResultDetails* details);

#ifdef __cplusplus
}
#endif

#endif // ADUC_RESULT_H
