/**
 * @file system_utils.h
 * @brief System level utilities, e.g. directory management, reboot, etc.
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#ifndef ADUC_SYSTEM_UTILS_H
#define ADUC_SYSTEM_UTILS_H

#include <aduc/c_utils.h>
#include <sys/types.h>

EXTERN_C_BEGIN

const char* ADUC_SystemUtils_GetTemporaryPathName();

int ADUC_SystemUtils_ExecuteShellCommand(const char* command);

int ADUC_SystemUtils_MkDir(const char* path, uid_t userId, gid_t groupId, mode_t mode);

int ADUC_SystemUtils_MkDirDefault(const char* path);

int ADUC_SystemUtils_MkDirRecursive(const char* path, uid_t userId, gid_t groupId, mode_t mode);

int ADUC_SystemUtils_MkDirRecursiveDefault(const char* path);

int ADUC_SystemUtils_RmDirRecursive(const char* path);

EXTERN_C_END

#endif // ADUC_SYSTEM_UTILS_H
