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
    // TODO (JeffMill): [PAL] Can't really do anything here for Windows. For now, return success.
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
    // TODO (JeffMill): [PAL] Can't really do anything here for Windows. For now, return 0.
    return 0;
}

uid_t ADUCPAL_geteuid()
{
    // TODO (JeffMill): [PAL] Can't really do anything here for Windows. For now, return 0.
    return 0;
}

pid_t ADUCPAL_getpid()
{
    return GetCurrentProcessId();
}

uid_t ADUCPAL_getuid()
{
    // TODO (JeffMill): [PAL] Can't really do anything here for Windows. For now, return 0.
    return 0;
}

int ADUCPAL_isatty(int fd)
{
    return _isatty(fd);
}

int ADUCPAL_open(const char* path, int oflag)
{
    int fd;
    int ret = _sopen_s(&fd, path, oflag, _SH_DENYNO, _S_IREAD | _S_IWRITE);
    if (ret != 0)
    {
        return -1;
    }
    return fd;
}

int ADUCPAL_rmdir(const char* path)
{
    return _rmdir(path);
}

int ADUCPAL_seteuid(uid_t uid)
{
    // TODO (JeffMill): [PAL] Can't really do anything here for Windows. For now, return success.
    UNREFERENCED_PARAMETER(uid);

    return 0;
}

int ADUCPAL_setegid(gid_t gid)
{
    // TODO (JeffMill): [PAL] Can't really do anything here for Windows. For now, return success.
    UNREFERENCED_PARAMETER(gid);

    return 0;
}

int ADUCPAL_setuid(uid_t uid)
{
    // TODO (JeffMill): [PAL] Can't really do anything here for Windows. For now, return success.
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

void ADUCPAL_sync()
{
    // TODO(JeffMill): [PAL] Requires admin privileges.

    // To flush all open files on a volume, call FlushFileBuffers with a handle to the volume.
    // The caller must have administrative privileges.
}

int ADUCPAL_unlink(const char* path)
{
    // If one or more processes have the file open when the last link is removed,
    // the link shall be removed before unlink() returns, but the removal of the file contents
    // shall be postponed until all references to the file are closed.

    // TODO(JeffMill): [PAL] _unlink on Windows doesn't have this behavior.
    // Perhaps h = CreateFile(file, ... FILE_FLAG_DELETE_ON_CLOSE); CloseHandle(h); ?

    return _unlink(path);
}
