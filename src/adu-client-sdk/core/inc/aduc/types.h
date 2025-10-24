/**
 * @file types.h
 * @brief Azure Device Update Core SDK Common Types
 * 
 * Common data types and structures used across all ADU extensions.
 * 
 * @copyright Copyright (C) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See LICENSE file in the project root for license information.
 */

#ifndef ADUC_TYPES_H
#define ADUC_TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Log levels for ADU extensions.
 */
typedef enum tagADUC_LogLevel
{
    ADUC_LOG_ERROR = 0,   /**< Error messages only */
    ADUC_LOG_WARN = 1,    /**< Warning and error messages */
    ADUC_LOG_INFO = 2,    /**< Informational, warning, and error messages */
    ADUC_LOG_DEBUG = 3    /**< All messages including debug */
} ADUC_LogLevel;

/**
 * @brief Update content type identifiers.
 */
typedef enum tagADUC_ContentType
{
    ADUC_ContentType_Unknown = 0,
    ADUC_ContentType_Bundle,      /**< Multi-file bundle */
    ADUC_ContentType_Firmware,    /**< Firmware image */
    ADUC_ContentType_Package,     /**< Software package (deb, rpm, etc.) */
    ADUC_ContentType_Script,      /**< Script file */
    ADUC_ContentType_Archive,     /**< Compressed archive */
    ADUC_ContentType_Delta,       /**< Delta/patch file */
    ADUC_ContentType_Manifest,    /**< Update manifest */
    ADUC_ContentType_Custom       /**< Custom/extension-specific type */
} ADUC_ContentType;

/**
 * @brief Update operation types.
 */
typedef enum tagADUC_UpdateAction
{
    ADUC_UpdateAction_Unknown = 0,
    ADUC_UpdateAction_Download,   /**< Download content */
    ADUC_UpdateAction_Install,    /**< Install content */
    ADUC_UpdateAction_Apply,      /**< Apply changes */
    ADUC_UpdateAction_Cancel,     /**< Cancel operation */
    ADUC_UpdateAction_Rollback    /**< Rollback to previous state */
} ADUC_UpdateAction;

/**
 * @brief Update state enumeration.
 */
typedef enum tagADUC_UpdateState
{
    ADUC_UpdateState_Idle = 0,           /**< No update in progress */
    ADUC_UpdateState_DownloadStarted,    /**< Download initiated */
    ADUC_UpdateState_DownloadSucceeded,  /**< Download completed successfully */
    ADUC_UpdateState_InstallStarted,     /**< Installation initiated */
    ADUC_UpdateState_InstallSucceeded,   /**< Installation completed successfully */
    ADUC_UpdateState_ApplyStarted,       /**< Apply initiated */
    ADUC_UpdateState_ApplySucceeded,     /**< Apply completed successfully */
    ADUC_UpdateState_Failed,             /**< Operation failed */
    ADUC_UpdateState_Cancelled           /**< Operation cancelled */
} ADUC_UpdateState;

/**
 * @brief String handle for managed strings.
 */
typedef struct tagADUC_String* ADUC_String_t;

/**
 * @brief File information structure.
 */
typedef struct tagADUC_FileInfo
{
    const char* fileName;       /**< File name */
    const char* filePath;       /**< Full file path */
    uint64_t fileSize;          /**< File size in bytes */
    const char* hash;           /**< File hash (SHA-256) */
    const char* hashAlgorithm;  /**< Hash algorithm identifier */
    ADUC_ContentType contentType; /**< Content type */
} ADUC_FileInfo;

/**
 * @brief Download information structure.
 */
typedef struct tagADUC_DownloadInfo
{
    const char* url;            /**< Download URL */
    const char* localPath;      /**< Local destination path */
    uint64_t expectedSize;      /**< Expected file size */
    const char* expectedHash;   /**< Expected file hash */
    const char* hashAlgorithm;  /**< Hash algorithm */
    int32_t timeoutSeconds;     /**< Download timeout */
    bool resumeSupported;       /**< Resume capability */
} ADUC_DownloadInfo;

/**
 * @brief Update identity structure.
 */
typedef struct tagADUC_UpdateId
{
    const char* provider;       /**< Update provider */
    const char* name;           /**< Update name */
    const char* version;        /**< Update version */
} ADUC_UpdateId;

/**
 * @brief Device information structure.
 */
typedef struct tagADUC_DeviceInfo
{
    const char* manufacturer;   /**< Device manufacturer */
    const char* model;          /**< Device model */
    const char* osType;         /**< Operating system type */
    const char* osVersion;      /**< Operating system version */
    const char* architecture;   /**< Processor architecture */
    const char* deviceId;       /**< Unique device identifier */
} ADUC_DeviceInfo;

/**
 * @brief Progress callback function type.
 * 
 * @param context User-provided context pointer
 * @param bytesTransferred Number of bytes transferred
 * @param totalBytes Total number of bytes to transfer
 * @param percentage Completion percentage (0-100)
 */
typedef void (*ADUC_ProgressCallback)(
    void* context,
    uint64_t bytesTransferred,
    uint64_t totalBytes,
    int32_t percentage);

/**
 * @brief Status callback function type.
 * 
 * @param context User-provided context pointer
 * @param state Current update state
 * @param message Status message (optional)
 */
typedef void (*ADUC_StatusCallback)(
    void* context,
    ADUC_UpdateState state,
    const char* message);

/**
 * @brief Log callback function type.
 * 
 * @param context User-provided context pointer
 * @param level Log level
 * @param message Log message
 */
typedef void (*ADUC_LogCallback)(
    void* context,
    ADUC_LogLevel level,
    const char* message);

#ifdef __cplusplus
}
#endif

#endif // ADUC_TYPES_H