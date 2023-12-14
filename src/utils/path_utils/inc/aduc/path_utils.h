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

EXTERN_C_BEGIN

STRING_HANDLE PathUtils_SanitizePathSegment(const char* unsanitized);

char* PathUtils_ConcatenateDirAndFolderPaths(const char* dirPath, const char* folderName);

EXTERN_C_END

#endif // ADUC_PATH_UTILS_H
