/**
 * @file file_info.h
 * @brief Enhanced file info structures for the download service.
 */
#ifndef ADUC_FILE_INFO_H
#define ADUC_FILE_INFO_H

#include "aduc/extension_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Target file properties (permissions, ownership).
 */
typedef struct ADUC_FileProperties
{
    const char* path;          ///< Override target path (NULL = use destDir/fileName)
    const char* permissions;   ///< POSIX permissions string, e.g., "0755"
    const char* owner;         ///< Target owner, e.g., "adu"
    const char* group;         ///< Target group, e.g., "adu"
    bool executable;           ///< Shorthand: set execute bit
} ADUC_FileProperties;

/**
 * @brief Related file (e.g., delta diff) metadata.
 */
typedef struct ADUC_RelatedFile
{
    const char* fileName;
    uint64_t    sizeInBytes;
    const char* hashAlgorithm;
    const char* hashValue;
    const char* downloadUrl;

    // Processor-specific properties
    const char* sourceHash;
    const char* sourceHashAlgorithm;
    const char* sourceVersion;

    ADUC_PropertyMap properties;  ///< Additional key-value properties
} ADUC_RelatedFile;

/**
 * @brief Enhanced file info for download service operations.
 */
typedef struct ADUC_FileInfo
{
    // Identity
    const char* fileId;
    const char* fileName;

    // Size & integrity
    uint64_t    sizeInBytes;
    const char* hashAlgorithm;       ///< "sha256"
    const char* hashValue;           ///< hex-encoded hash

    // Download source
    const char* downloadUrl;         ///< Direct URL (NULL = resolve via comm provider)
    const char* downloadHandlerId;   ///< Content processor capability (e.g., "microsoft/delta:2")

    // Target file properties
    ADUC_FileProperties targetProperties;

    // Related files (for content processors like delta)
    ADUC_RelatedFile* relatedFiles;
    size_t relatedFileCount;

    // Runtime state (set by download service after download)
    char* localPath;                 ///< Actual local path (NULL before download)

} ADUC_FileInfo;

#ifdef __cplusplus
}
#endif

#endif // ADUC_FILE_INFO_H
