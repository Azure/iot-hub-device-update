/**
 * @file system_utils.c
 * @brief System level utilities, e.g. directory management, reboot, etc.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/system_utils.h"
#include "aduc/logging.h"

// for nftw
#define __USE_XOPEN_EXTENDED 1

#include <aduc/string_c_utils.h>
#include <azure_c_shared_utility/strings.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h> // for O_CLOEXEC
#include <ftw.h> // for nftw
#include <grp.h> // for getgrnam
#include <limits.h> // for PATH_MAX
#include <pwd.h> // for getpwnam
#include <stdio.h>
#include <stdlib.h> // for getenv
#include <string.h> // for strncpy, strlen
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h> // for waitpid
#include <unistd.h>

#ifndef O_CLOEXEC
/**
 * @brief Enable the close-on-exec flag for the new file descriptor. Specifying this flag permits a program to avoid additional
 * fcntl(2) F_SETFD operations to set the FD_CLOEXEC flag.
 * @details Included here because not all linux kernels include O_CLOEXEC by default in fcntl.h
 */
#    define O_CLOEXEC __O_CLOEXEC
#endif

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
 * @brief Execute shell command.
 *
 * @param command The command to execute as a string.
 * @return 0 for success.
 * @return non-zero status code for failure.
 *
 * The return value is an errno code for system call failures. If all system calls succeed,
 * the return value is the termination status of the child shell used to execute command.
 */
int ADUC_SystemUtils_ExecuteShellCommand(const char* command)
{
    if (IsNullOrEmpty(command))
    {
        Log_Error("ExecuteShellCommand failed: command is empty");
        return EINVAL;
    }

    Log_Info("Execute shell command: %s", command);

    const int status = system(command); // NOLINT(cert-env33-c)
    if (status == -1)
    {
        Log_Error("ExecuteShellCommand failed: System call failed, errno = %d", errno);

        return errno;
    }

    // WIFEXITED returns zero if the child process terminated abnormally.
    if (!WIFEXITED(status))
    {
        Log_Error("ExecuteShellCommand failed: Command exited abnormally");

        return ECANCELED;
    }

    // if child process terminated normally, WEXITSTATUS returns its exit status value.
    const int executeStatus = WEXITSTATUS(status);

    // A shell command which exits with a zero exit status has succeeded. A non-zero exit status indicates failure.
    if (executeStatus != 0)
    {
        Log_Error("ExecuteShellCommand failed: Command exited with non-zero value, exitStatus = %d", executeStatus);

        return executeStatus;
    }

    return 0;
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
    return ADUC_SystemUtils_MkDir(path, -1 /*userId*/, -1 /*groupId*/, S_IRWXU | S_IRGRP | S_IWGRP | S_IXGRP /*mode*/);
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
    struct stat st = {};
    if (stat(path, &st) != 0)
    {
        /* Directory does not exist. EEXIST for race condition */
        if (mkdir(path, mode) != 0 && errno != EEXIST)
        {
            Log_Error("Could not create directory %s errno: %d", path, errno);
            return errno;
        }

        if (groupId != -1 || userId != -1)
        {
            // Now that we have created the directory, take ownership of it.
            // Note: getuid and getgid are always successful.
            // getuid(), getgid()
            if (chown(path, userId, groupId) != 0)
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
    return ADUC_SystemUtils_MkDirRecursive(path, -1 /*userId*/, -1 /*groupId*/, S_IRWXU | S_IRWXG /*mode*/);
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
    const size_t mkdirPath_cch = ADUC_StrNLen(path, PATH_MAX);

    // Create writable copy of path to iterate over.
    if (mkdirPath_cch + 1 > ARRAY_SIZE(mkdirPath))
    {
        return ENAMETOOLONG;
    }

    strncpy(mkdirPath, path, mkdirPath_cch);
    mkdirPath[mkdirPath_cch] = '\0';

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
    struct stat st = {};
    if (stat(path, &st) == 0)
    {
        int perms = (st.st_mode & (ALL_PERMS));
        if (perms != mode)
        {
            // Fix the permissions.
            if (0 != chmod(path, mode))
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
    struct passwd* pwd = getpwnam(ADUC_FILE_USER);
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
    struct group* grp = getgrnam(ADUC_FILE_GROUP);
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
    struct passwd* pwd = getpwnam(ADUC_FILE_USER);
    if (pwd == NULL)
    {
        Log_Error("adu user doesn't exist.");
        return -1;
    }

    uid_t aduUserId = pwd->pw_uid;
    pwd = NULL;

    return ADUC_SystemUtils_MkDirRecursive(path, aduUserId, -1, S_IRWXU);
}

static int RmDirRecursive_helper(const char* fpath, const struct stat* sb, int typeflag, struct FTW* info)
{
    UNREFERENCED_PARAMETER(sb);
    UNREFERENCED_PARAMETER(info);

    int result = 0;

    if (typeflag == FTW_DP)
    {
        // fpath is a directory, and FTW_DEPTH was specified in flags.
        result = rmdir(fpath);
    }
    else
    {
        // Assume a file.
        result = unlink(fpath);
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
    return nftw(path, RmDirRecursive_helper, 20 /*nfds*/, FTW_MOUNT | FTW_PHYS | FTW_DEPTH);
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
    if (dirPath[dirPathSize - 2] != '/')
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
    const size_t readMaxBuffSize = 1024;
    unsigned char readBuff[readMaxBuffSize];

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

    if (chmod(STRING_c_str(destFilePath), buff.st_mode) != 0)
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
 * @brief Writes @p buff to file at @p path using
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

    dir = opendir(baseDir);
    if (dir == NULL)
    {
        err_ret = errno;
        Log_Error("opendir '%s' failed: %d", baseDir, err_ret);
        goto done;
    }

    do
    {
        errno = 0;
        if ((dir_entry = readdir(dir)) != NULL)
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
        closedir(dir);
        dir = NULL;
    }

    return err_ret;
}
