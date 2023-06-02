#include "aducpal/unistd.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <direct.h> // rmdir
#include <io.h> // isatty, unlink, open, close
#include <sys/stat.h> // _S_*

#ifndef UNREFERENCED_PARAMETER
#    define UNREFERENCED_PARAMETER(param) (void)(param)
#endif

int ADUCPAL_access(const char* pathname, int mode)
{
    if (mode != F_OK)
    {
        // Note: Only supporting F_OK (file existence)
        _set_errno(EINVAL);
        return -1;
    }

    // Returns 0 on success, -1 and sets errno on failure.
    struct stat st;
    return stat(pathname, &st);
}

int ADUCPAL_chown(const char* path, uid_t owner, gid_t group)
{
    // For Windows, just return 0.
    UNREFERENCED_PARAMETER(path);
    UNREFERENCED_PARAMETER(owner);
    UNREFERENCED_PARAMETER(group);

    return 0;
}

int ADUCPAL_close(int fildes)
{
    return _close(fildes);
}

gid_t ADUCPAL_getegid()
{
    // For Windows, just return 0.
    return 0;
}

uid_t ADUCPAL_geteuid()
{
    // For Windows, just return 0.
    return 0;
}

pid_t ADUCPAL_getpid()
{
    return GetCurrentProcessId();
}

uid_t ADUCPAL_getuid()
{
    // For Windows, just return 0.
    return 0;
}

int ADUCPAL_isatty(int fd)
{
    return _isatty(fd);
}

int ADUCPAL_rmdir(const char* path)
{
    return _rmdir(path);
}

int ADUCPAL_seteuid(uid_t uid)
{
    // For Windows, just return 0.
    UNREFERENCED_PARAMETER(uid);

    return 0;
}

int ADUCPAL_setegid(gid_t gid)
{
    // For Windows, just return 0.
    UNREFERENCED_PARAMETER(gid);

    return 0;
}

int ADUCPAL_setuid(uid_t uid)
{
    // For Windows, just return 0.
    UNREFERENCED_PARAMETER(uid);

    return 0;
}

long ADUCPAL_syscall(long number)
{
    if (number == SYS_gettid)
    {
        return GetCurrentThreadId();
    }

    // Note: Only SYS_gettid is supported.
    _set_errno(ENOSYS);
    return -1;
}
