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

typedef void (*ADUC_SystemUtils_ForEachDirFunc)(void* context, const char* baseDir, const char* subDir);

/**
 * @brief A function object for use with SystemUtils_ForEachDir with optional context.
 *
 */
typedef struct tagADUC_SystemUtils_ForEachDirFunctor
{
    void* context; ///< The context for the first argument of callbackFn.
    ADUC_SystemUtils_ForEachDirFunc callbackFn; ///< The ForEachDirFunc callback function.
} ADUC_SystemUtils_ForEachDirFunctor;

const char* ADUC_SystemUtils_GetTemporaryPathName();

int ADUC_SystemUtils_ExecuteShellCommand(const char* command);

int ADUC_SystemUtils_MkDir(const char* path, uid_t userId, gid_t groupId, mode_t mode);

int ADUC_SystemUtils_MkDirDefault(const char* path);

int ADUC_SystemUtils_MkDirRecursive(const char* path, uid_t userId, gid_t groupId, mode_t mode);

int ADUC_SystemUtils_MkDirRecursiveDefault(const char* path);

int ADUC_SystemUtils_MkSandboxDirRecursive(const char* path);

int ADUC_SystemUtils_MkDirRecursiveAduUser(const char* path);

int ADUC_SystemUtils_RmDirRecursive(const char* path);

int ADUC_SystemUtils_CopyFileToDir(const char* filePath, const char* dirPath, bool overwriteExistingFile);

int ADUC_SystemUtils_RemoveFile(const char* path);

int ADUC_SystemUtils_WriteStringToFile(const char* path, const char* buff);

int ADUC_SystemUtils_ReadStringFromFile(const char* path, char* buff, size_t buffLen);

bool SystemUtils_IsDir(const char* path, int* err);

bool SystemUtils_IsFile(const char* path, int* err);

int SystemUtils_ForEachDir(
    const char* baseDir, const char* excludeDir, ADUC_SystemUtils_ForEachDirFunctor* perDirActionFunctor);

EXTERN_C_END

#endif // ADUC_SYSTEM_UTILS_H
