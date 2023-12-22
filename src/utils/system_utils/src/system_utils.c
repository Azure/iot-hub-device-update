/**
  * @file system_utils.c
 * @brief System level utilities, e.g. directory management, reboot, etc.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/system_utils.h"
#include "aduc/logging.h"

#include <aducpal/dirent.h>
#include <aducpal/ftw.h> // nftw
#include <aducpal/grp.h> // getgrnam
#include <aducpal/pwd.h> // getpwnam
#include <aducpal/stdio.h> // remove
#include <aducpal/sys_stat.h> // mkdir, chmod
#include <aducpal/time.h> // clock_gettime, CLOCK_REALTIME
#include <aducpal/unistd.h> // chown

#include <aduc/string_c_utils.h>
#include <azure_c_shared_utility/strings.h>
#include <aducpal/dirent.h>
#include <errno.h>
#include <limits.h> // for PATH_MAX
#include <stdio.h>
#include <stdlib.h> // for getenv
#include <string.h> // strlen
#include <sys/stat.h>
#include <sys/types.h>

// keep this last to avoid interfering with system headers
#include "aduc/aduc_banned.h"

#ifndef ALL_PERMS
/**
 * @brief Define all permissions mask if not defined already in octal format
 */
#    define ALL_PERMS 07777
#endif

/**
 * @brief Retrieve system temporary path with a subfolder.
 *
 * This only returns a folder name, which is neither created nor checked for existence.
 *
 * Loosely based on Boost's implementation, which is:
 * TMPDIR > TMP > TEMP > TEMPDIR > (android only) "/data/local/tmp" > "/tmp"
 *
 * @return const char* Returns the path to the temporary directory. This is a long-lived string.
 */
const char* ADUC_SystemUtils_GetTemporaryPathName()
{
    const char* env;
    env = getenv("TMPDIR");
    if (env == NULL)
    {
        env = getenv("TMP");
        if (env == NULL)
        {
            env = getenv("TEMP");
            if (env == NULL)
            {
                env = getenv("TEMPDIR");
                if (env == NULL)
                {
                    static char* root_tmp_folder = "/tmp";
                    env = root_tmp_folder;
                }
            }
        }
    }

    return env;
}

/**
 * @brief mktemp() implementation
 *
 * mktemp() is generally considered unsafe as the temporary names can be easy to guess, and there are
 * a limited number of combinations.
 *
 * However, for unit tests, it can be useful, so it's highly recommended that this method is only
 * used for unit tests.
 *
 * @param tmpl The last six characters XXXXXX are replaced with a string that makes the filename unique.
 * @return Returns @param tmpl.
 */
char* ADUC_SystemUtils_MkTemp(char* tmpl)
{
    const size_t len = strlen(tmpl);
    if (len < 6)
    {
        tmpl[0] = '\0';
        errno = EINVAL;
        return tmpl;
    }

    // Start of XXXXXX
    char* Xs = tmpl + (len - 6);

    // Get a randomish number from real time clock.
    struct timespec ts;
    ADUCPAL_clock_gettime(CLOCK_REALTIME, &ts);
    unsigned long rnd = (unsigned long)ts.tv_nsec ^ (unsigned long)ts.tv_sec;

    while (*Xs == 'X')
    {
        // '0'..'9' (10), 'A'..'Z' (26), 'a'..'z' (26)
        const unsigned short count = 10 + 26 + 26;
        const unsigned short idx = (unsigned short)(rnd % count);
        *Xs = (idx < 10) ? (char)('0' + idx)
                         : (idx < 10 + 26) ? (char)('A' + (idx - 10)) : (char)('a' + (idx - (10 + 26)));
        rnd /= count;

        ++Xs;
    }

    return tmpl;
}

/**
 * @brief Wrapper for mkdir that handles a number of common conditions, e.g. don't fail if directory already exists.
 *
 * Calls ADUC_SystemUtils_MkDir with:
 * userId == -1 (system default behavior)
 * groupId == -1 (system default behavior)
 * mode == S_IRWXU | S_IRGRP | S_IWGRP | S_IXGRP
 *
 * @param path Path to create.
 *
 * @return errno, zero on success.
 */
int ADUC_SystemUtils_MkDirDefault(const char* path)
{
    return ADUC_SystemUtils_MkDir(
        path, (uid_t)-1 /*userId*/, (gid_t)-1 /*groupId*/, (mode_t)S_IRWXU | S_IRGRP | S_IWGRP | S_IXGRP /*mode*/);
}

/**
 * @brief Wrapper for mkdir that handles a number of common conditions, e.g. don't fail if directory already exists.
 *
 * @param path The path to the directory to create.
 * @param userId The uid_t of the user owner the new directory. -1 to indicate the system default behavior.
 * @param groupId The gid_t of the group owner of the new directory. -1 to indicate the system default behavior.
 * @param mode The mode_t of the desired permissions of the new directory.
 *
 * @return errno, zero on success.
 */
int ADUC_SystemUtils_MkDir(const char* path, uid_t userId, gid_t groupId, mode_t mode)
{
    struct stat st;
    memset(&st, 0, sizeof(st));

    if (stat(path, &st) != 0)
    {
        /* Directory does not exist. EEXIST for race condition */
        if (ADUCPAL_mkdir(path, mode) != 0 && errno != EEXIST)
        {
            Log_Error("Could not create directory %s errno: %d", path, errno);
            return errno;
        }

        if (groupId != -1 || userId != -1)
        {
            // Now that we have created the directory, take ownership of it.
            // Note: getuid and getgid are always successful.
            // getuid(), getgid()
            if (ADUCPAL_chown(path, userId, groupId) != 0)
            {
                Log_Error("Could not change owner of directory %s errno: %d", path, errno);
                return errno;
            }
        }
    }
    else if (S_ISDIR(st.st_mode) == 0)
    {
        errno = ENOTDIR;
        Log_Error("Path was not a directory %s errno: %d", path, errno);
        return errno;
    }

    return 0;
}

/**
 * @brief Create a directory tree.
 *
 * Calls ADUC_SystemUtils_MkDirRecursive with:
 * userId == -1 (system default behavior)
 * groupId == -1 (system default behavior)
 * mode == S_IRWXU | S_IRWXG
 *
 * @param path Path to create.
 *
 * @return errno, zero on success.
 */
int ADUC_SystemUtils_MkDirRecursiveDefault(const char* path)
{
    return ADUC_SystemUtils_MkDirRecursive(
        path, (uid_t)-1 /*userId*/, (gid_t)-1 /*groupId*/, (mode_t)S_IRWXU | S_IRWXG /*mode*/);
}

/**
 * @brief Checks if the file or directory at @p path exists
 *
 * @param path the path to the directory or file
 * @return true if path exists, false otherwise
 */
bool ADUC_SystemUtils_Exists(const char* path)
{
    if (path == NULL)
    {
        return false;
    }

    int result = 0;
    struct stat buff;

    result  = stat(path,&buff);

    if (result != 0)
    {
        return false;
    }

    return true;
}
/**
 * @brief Create a directory tree.
 *
 * On error, none of the partially created tree is cleaned up.
 *
 * @param path Path to create.
 * @param userId The uid_t of the user owner any new directory. -1 to indicate the system default behavior.
 * @param groupId The gid_t of the group owner any new directory. -1 to indicate the system default behavior.
 * @param mode The mode_t of the desired permissions any new directory.
 *
 * @return errno, zero on success.
 */
int ADUC_SystemUtils_MkDirRecursive(const char* path, uid_t userId, gid_t groupId, mode_t mode)
{
    if (path == NULL)
    {
        return EINVAL;
    }

    char mkdirPath[PATH_MAX + 1];
    memset(mkdirPath, 0, sizeof(mkdirPath));
    const size_t mkdirPath_cch = ADUC_StrNLen(path, PATH_MAX);
    ADUC_Safe_StrCopyN(mkdirPath, path, sizeof(mkdirPath), mkdirPath_cch);

    // Remove any trailing slash.
    if (mkdirPath[mkdirPath_cch - 1] == '/')
    {
        mkdirPath[mkdirPath_cch - 1] = '\0';
    }

    for (char* current = (mkdirPath[0] == '/') ? (mkdirPath + 1) : mkdirPath; *current != '\0'; ++current)
    {
        if (*current == '/')
        {
            *current = '\0';

            const int ret = ADUC_SystemUtils_MkDir(mkdirPath, userId, groupId, mode);
            if (ret != 0)
            {
                return ret;
            }

            *current = '/';
        }
    }

    const int ret = ADUC_SystemUtils_MkDir(mkdirPath, userId, groupId, mode);
    if (ret != 0)
    {
        return ret;
    }

    // Let's make sure that we set this correct permission on the leaf folder.
    struct stat st;
    memset(&st, 0, sizeof(st));

    if (stat(path, &st) == 0)
    {
        unsigned short perms = (st.st_mode & (ALL_PERMS));
        if (perms != mode)
        {
            // Fix the permissions.
            if (0 != ADUCPAL_chmod(path, mode))
            {
                stat(path, &st);
                Log_Warn("Failed to set '%s' folder permissions (expected:0%o, actual: 0%o)", mkdirPath, mode, perms);
            }
        }
    }

    return 0;
}

int ADUC_SystemUtils_MkSandboxDirRecursive(const char* path)
{
    // Create the sandbox folder with adu:adu ownership.
    // Permissions are set to u=rwx,g=rwx. We grant read/write/execute to group owner so that partner
    // processes like the DO daemon can download files to our sandbox.

    // Note: the return value may point to a static area,
    // and may be overwritten by subsequent calls to getpwent(3), getpwnam(), or getpwuid().
    // (Do not pass the returned pointer to free(3).)
    struct passwd* pwd = ADUCPAL_getpwnam(ADUC_FILE_USER);
    if (pwd == NULL)
    {
        Log_Error("adu user doesn't exist.");
        return -1;
    }

    uid_t aduUserId = pwd->pw_uid;
    pwd = NULL;

    // Note: The return value may point to a static area,
    // and may be overwritten by subsequent calls to getgrent(3), getgrgid(), or getgrnam().
    // (Do not pass the returned pointer to free(3).)
    struct group* grp = ADUCPAL_getgrnam(ADUC_FILE_GROUP);
    if (grp == NULL)
    {
        Log_Error("adu group doesn't exist.");
        return -1;
    }
    gid_t aduGroupId = grp->gr_gid;
    grp = NULL;

    return ADUC_SystemUtils_MkDirRecursive(path, aduUserId, aduGroupId, S_IRWXU | S_IRWXG);
}

int ADUC_SystemUtils_MkDirRecursiveAduUser(const char* path)
{
    // Create the sandbox folder with adu:default ownership.
    // Permissions are set to u=rwx.
    struct passwd* pwd = ADUCPAL_getpwnam(ADUC_FILE_USER);
    if (pwd == NULL)
    {
        Log_Error("adu user doesn't exist.");
        return -1;
    }

    uid_t aduUserId = pwd->pw_uid;
    pwd = NULL;

    return ADUC_SystemUtils_MkDirRecursive(path, aduUserId, (gid_t)-1, (mode_t)S_IRWXU);
}

static int RmDirRecursive_helper(const char* fpath, const struct stat* sb, int typeflag, struct FTW* info)
{
    UNREFERENCED_PARAMETER(sb);
    UNREFERENCED_PARAMETER(info);

    int result = 0;

    if (typeflag == FTW_DP)
    {
        // fpath is a directory, and FTW_DEPTH was specified in flags.
        result = ADUCPAL_rmdir(fpath);
    }
    else
    {
        // Assume a file.
        result = ADUCPAL_remove(fpath);
    }

    return result;
}

/**
 * @brief Remove a directory recursively.
 *
 * Directories are handled before the files and subdirectories they contain (preorder traversal).
 * If fn() returns nonzero, then the tree walk is terminated and the value returned by fn()
 * is returned as the result of nftw().
 *
 * @return int errno, 0 if success.
 */
int ADUC_SystemUtils_RmDirRecursive(const char* path)
{
    return ADUCPAL_nftw(path, RmDirRecursive_helper, 20 /*nfds*/, FTW_MOUNT | FTW_PHYS | FTW_DEPTH);
}

/**
 * @brief Takes the filename from @p filePath and concatenates it with @p dirPath and stores the result in @p newFilePath
 * @details newFilePath should be freed using STRING_delete() by caller
 * @param[out] newFilePath handle that will be allocated and intiialized with the new file path
 * @param[in] filePath the path to a file who's name will be stripped out
 * @param[in] dirPath directoryPath to have @p filePath's file name appended to it
 * @returns true on success; false otherwise
 */
bool ADUC_SystemUtils_FormatFilePathHelper(STRING_HANDLE* newFilePath, const char* filePath, const char* dirPath)
{
    if (newFilePath == NULL || filePath == NULL || dirPath == NULL)
    {
        return false;
    }

    bool succeeded = false;
    size_t dirPathSize = strlen(dirPath);

    STRING_HANDLE tempHandle = STRING_new();

    bool needForwardSlash = false;
    if (dirPath[dirPathSize - 1] != '/')
    {
        needForwardSlash = true;
    }

    const char* fileNameStart = strrchr(filePath, '/');

    if (fileNameStart == NULL)
    {
        goto done;
    }
    const size_t fileNameSize = strlen(fileNameStart) - 1;

    if (fileNameSize == 0)
    {
        goto done;
    }

    // increment past the '/'
    ++fileNameStart;

    if (fileNameSize == 0 || dirPathSize + fileNameSize > PATH_MAX)
    {
        goto done;
    }

    if (needForwardSlash)
    {
        if (STRING_sprintf(tempHandle, "%s/%s", dirPath, fileNameStart) != 0)
        {
            goto done;
        }
    }
    else
    {
        if (STRING_sprintf(tempHandle, "%s%s", dirPath, fileNameStart) != 0)
        {
            goto done;
        }
    }

    succeeded = true;
done:

    if (!succeeded)
    {
        STRING_delete(tempHandle);
        tempHandle = NULL;
    }

    *newFilePath = tempHandle;

    return succeeded;
}

/**
 * @brief Copies the file at @p filePath to @p dirPath with the same name
 * @details Preserves the ownership and filemode bit permissions
 * @param filePath path to the file
 * @param dirPath path to the directory
 * @param overwriteExistingFile if set to true will overwrite the existing file in @p dirPath named with the filename in @p fileName if it exists
 * @returns the result of the operation
 */
int ADUC_SystemUtils_CopyFileToDir(const char* filePath, const char* dirPath, const bool overwriteExistingFile)
{
    int result = -1;
    STRING_HANDLE destFilePath = NULL;

    FILE* sourceFile = NULL;
    FILE* destFile = NULL;
    unsigned char readBuff[1024];
    const size_t readMaxBuffSize = ARRAY_SIZE(readBuff);

    memset(readBuff, 0, readMaxBuffSize);

    if (filePath == NULL || dirPath == NULL)
    {
        goto done;
    }

    if (!ADUC_SystemUtils_FormatFilePathHelper(&destFilePath, filePath, dirPath))
    {
        goto done;
    }

    sourceFile = fopen(filePath, "rb");

    if (sourceFile == NULL)
    {
        goto done;
    }

    if (overwriteExistingFile)
    {
        destFile = fopen(STRING_c_str(destFilePath), "wb+");
    }
    else
    {
        destFile = fopen(STRING_c_str(destFilePath), "wb");
    }

    if (destFile == NULL)
    {
        goto done;
    }

    size_t readBytes = fread(readBuff, readMaxBuffSize, sizeof(readBuff[0]), sourceFile);

    while (readBytes != 0 && feof(sourceFile) != 0)
    {
        const size_t writtenBytes = fwrite(readBuff, readBytes, sizeof(readBuff[0]), destFile);

        result = ferror(destFile);
        if (writtenBytes != readBytes || result != 0)
        {
            goto done;
        }

        readBytes = fread(readBuff, readMaxBuffSize, sizeof(readBuff[0]), sourceFile);
        result = ferror(sourceFile);
        if (result != 0)
        {
            goto done;
        }
    }

    struct stat buff;
    if (stat(filePath, &buff) != 0)
    {
        goto done;
    }

    if (ADUCPAL_chmod(STRING_c_str(destFilePath), buff.st_mode) != 0)
    {
        goto done;
    }

    result = 0;
done:

    fclose(sourceFile);
    fclose(destFile);

    if (result != 0)
    {
        remove(STRING_c_str(destFilePath));
    }

    STRING_delete(destFilePath);
    return result;
}

/**
 * @brief Removes the file when caller knows the path refers to a file
 * @remark On POSIX systems, it will remove a link to the name so it might not delete right away if there are other links
 * or another process has it open.
 *
 * @param path The path to the file.
 * @return int On success, 0 is returned. On error -1 is returned, and errno is set appropriately.
 */
int ADUC_SystemUtils_RemoveFile(const char* path)
{
    return unlink(path);
}

/**
 * @brief Writes @p buff to file at @p path
 * @details This function overwrites the current data in @p path with @p buff
 *
 * @param path the path to the file to write data
 * @param buff data to be written to the file
 * @return in On success 0 is returned; otherwise errno or -1 on failure
 */
int ADUC_SystemUtils_WriteStringToFile(const char* path, const char* buff)
{
    int status = -1;
    FILE* file = NULL;

    if (path == NULL || buff == NULL)
    {
        goto done;
    }

    const size_t bytesToWrite = strlen(buff);

    if (bytesToWrite == 0)
    {
        return -1;
    }

    file = fopen(path, "w");

    if (file == NULL)
    {
        status = errno;
        goto done;
    }

    const size_t writtenBytes = fwrite(buff, sizeof(char), bytesToWrite, file);

    if (writtenBytes != bytesToWrite)
    {
        status = -1;
        goto done;
    }

    status = 0;

done:

    if (file != NULL)
    {
        fclose(file);
    }

    return status;
}

/**
 * @brief Reads the data from @p path into @p buff up to @p buffLen bytes
 * @details Only up to @p buffLen - 1 bytes will be read from the file to save space for the null terminator
 * @param path path to the file to read from
 * @param buff buffer for the data to go into
 * @param buffLen the maximum amount of data to be written to buff including the null-terminator
 * @return returns 0 on success; errno or -1 on failure
 */
int ADUC_SystemUtils_ReadStringFromFile(const char* path, char* buff, size_t buffLen)
{
    int status = -1;
    FILE* file = NULL;

    if (path == NULL || buff == NULL || buffLen < 2)
    {
        goto done;
    }

    file = fopen(path, "r");

    if (file == NULL)
    {
        status = errno;
        goto done;
    }

    const size_t readBytes = fread(buff, sizeof(char), buffLen - 1, file);

    if (readBytes == 0)
    {
        if (feof(file) != 0)
        {
            status = -1;
            goto done;
        }

        if (ferror(file) != 0)
        {
            status = errno;
            goto done;
        }
    }

    buff[readBytes] = '\0';

    status = 0;

done:

    if (file != NULL)
    {
        fclose(file);
    }

    return status;
}

/**
 * @brief Checks if the file object at the given path is a directory.
 * @param path The path.
 * @param err the error code (optional, can be NULL)
 * @returns true if it is a directory, false otherwise.
 */
bool SystemUtils_IsDir(const char* path, int* err)
{
    bool is_dir = false;
    int err_ret = 0;

    struct stat st;
    if (stat(path, &st) != 0)
    {
        err_ret = errno;
        Log_Error("stat path '%s' failed: %d", path, err_ret);
        is_dir = false;
        goto done;
    }

    if (!S_ISDIR(st.st_mode))
    {
        is_dir = false;
        goto done;
    }

    is_dir = true;

done:
    if (err != NULL)
    {
        *err = err_ret;
    }
    return is_dir;
}

/**
 * @brief Checks if the file object at the given path is a file.
 * @param path The path.
 * @param err the error code (optional, can be NULL)
 * @returns true if it is a file, false otherwise.
 */
bool SystemUtils_IsFile(const char* path, int* err)
{
    bool is_file = false;
    int err_ret = 0;

    struct stat st;
    if (stat(path, &st) != 0)
    {
        err_ret = errno;
        Log_Error("stat path '%s' failed: %d", path, err_ret);
        is_file = false;
        goto done;
    }

    if (!S_ISREG(st.st_mode))
    {
        is_file = false;
        goto done;
    }

    is_file = true;

done:
    if (err != NULL)
    {
        *err = err_ret;
    }
    return is_file;
}

/**
 * @brief For each dir_name in the baseDir, it calls the action function with the baseDir and dir_name.
 * @param baseDir The base dir to list directories in.
 * @param excludedDir A dir to exclude via exact match in addition to . and .. dirs. NULL means to exclude only . and .. dirs.
 * @param perDirActionFunctor The functor to apply for each dir in the baseDir.
 * @returns 0 if succeeded in calling the action func for every dir in baseDir (except . and .. dirs).
 */
int SystemUtils_ForEachDir(
    const char* baseDir, const char* excludedDir, ADUC_SystemUtils_ForEachDirFunctor* perDirActionFunctor)
{
    int err_ret = -1;
    struct dirent* dir_entry = NULL;
    DIR* dir = NULL;

    if (baseDir == NULL || (perDirActionFunctor == NULL) || (perDirActionFunctor->callbackFn == NULL))
    {
        goto done;
    }

    dir = ADUCPAL_opendir(baseDir);
    if (dir == NULL)
    {
        err_ret = errno;
        Log_Error("opendir '%s' failed: %d", baseDir, err_ret);
        goto done;
    }

    do
    {
        errno = 0;
        if ((dir_entry = ADUCPAL_readdir(dir)) != NULL)
        {
            if (strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0)
            {
                continue;
            }

            if (excludedDir != NULL && strcmp(dir_entry->d_name, excludedDir) == 0)
            {
                continue;
            }

            perDirActionFunctor->callbackFn(perDirActionFunctor->context, baseDir, &(dir_entry->d_name)[0]);
        }
        else // dir_entry == NULL
        {
            if (errno != 0)
            {
                // error
                err_ret = errno;
                goto done;
            }
            // otherwise, it's the end of the stream.
        }
    } while (dir_entry != NULL);

    err_ret = 0;
done:
    if (dir != NULL)
    {
        ADUCPAL_closedir(dir);
        dir = NULL;
    }

    return err_ret;
}
