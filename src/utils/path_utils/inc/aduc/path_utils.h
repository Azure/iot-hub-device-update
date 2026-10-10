/**
 * @file path_utils.h
 * @brief Utilities for working with paths.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_PATH_UTILS_H
#define ADUC_PATH_UTILS_H

#include <aduc/c_utils.h> // EXTERN_C_BEGIN, EXTERN_C_END
#include <azure_c_shared_utility/strings.h> // STRING_HANDLE
#include <stdbool.h>

EXTERN_C_BEGIN

STRING_HANDLE PathUtils_SanitizePathSegment(const char* unsanitized);

char* PathUtils_ConcatenateDirAndFolderPaths(const char* dirPath, const char* folderName);

/**
 * @brief Checks that appending a path to a directory does not traverse above that directory.
 * @param path The unmodified suffix appended after a directory separator, not a standalone absolute path.
 * @return true for nonempty paths whose parent components never climb above the starting directory.
 * @remark Preserves nested paths and contained parent components. Does not resolve symlinks or access the filesystem.
 */
bool PathUtils_IsPathLexicallyContained(const char* path);

EXTERN_C_END

#endif // ADUC_PATH_UTILS_H
