/**
 * @file system_utils.c
 * @brief System level utilities, e.g. directory management, reboot, etc.
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#include "aduc/system_utils.h"
#include "aduc/logging.h"

// for nftw
#define __USE_XOPEN_EXTENDED 1

#include <errno.h>
#include <ftw.h> // for nftw
#include <limits.h> // for PATH_MAX
#include <stdlib.h> // for getenv
#include <string.h> // for strncpy, strlen
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h> // for waitpid
#include <unistd.h>

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
    if (command == NULL || command[0] == '\0')
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
 * mode == S_IRWXU | S_IRGRP | S_IWGRP | S_IXGRP
 *
 * @param path Path to create.
 *
 * @return errno, zero on success.
 */
int ADUC_SystemUtils_MkDirRecursiveDefault(const char* path)
{
    return ADUC_SystemUtils_MkDirRecursive(
        path, -1 /*userId*/, -1 /*groupId*/, S_IRWXU | S_IRGRP | S_IWGRP | S_IXGRP /*mode*/);
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
    const size_t mkdirPath_cch = strnlen(path, PATH_MAX);

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
        int perms = st.st_mode & ~S_IFMT;
        if (perms != mode)
        {
            // Fix the permissions.
            if (0 != chmod(path, mode))
            {
                stat(path, &st);
                Log_Warn(
                    "Failed to set '%s' folder permissions (expected:%d, actual: %d)",
                    mkdirPath,
                    mode,
                    st.st_mode & ~S_IFMT);
            }
        }
    }

    return 0;
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

