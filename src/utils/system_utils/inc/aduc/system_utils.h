/**
 * @file system_utils.h
 * @brief System level utilities, e.g. directory management, reboot, etc.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_SYSTEM_UTILS_H
#define ADUC_SYSTEM_UTILS_H

#include <aduc/c_utils.h>
#include <stdbool.h>
#include <sys/types.h>

EXTERN_C_BEGIN

const char* ADUC_SystemUtils_GetTemporaryPathName();

int ADUC_SystemUtils_ExecuteShellCommand(const char* command);

int ADUC_SystemUtils_MkDir(const char* path, uid_t userId, gid_t groupId, mode_t mode);

int ADUC_SystemUtils_MkDirDefault(const char* path);

int ADUC_SystemUtils_MkDirRecursive(const char* path, uid_t userId, gid_t groupId, mode_t mode);

int ADUC_SystemUtils_MkDirRecursiveDefault(const char* path);

int ADUC_SystemUtils_MkSandboxDirRecursive(const char* path);

int ADUC_SystemUtils_MkDirRecursiveAduUser(const char* path);

int ADUC_SystemUtils_RmDirRecursive(const char* path);

int ADUC_SystemUtils_CopyFileToDir(const char* filePath, const char* dirPath, _Bool overwriteExistingFile);

int ADUC_SystemUtils_RemoveFile(const char* path);

int ADUC_SystemUtils_WriteStringToFile(const char* path, const char* buff);

int ADUC_SystemUtils_ReadStringFromFile(const char* path, char* buff, size_t buffLen);

_Bool SystemUtils_IsDir(const char* path, int* err);

_Bool SystemUtils_IsFile(const char* path, int* err);

EXTERN_C_END

#endif // ADUC_SYSTEM_UTILS_H
