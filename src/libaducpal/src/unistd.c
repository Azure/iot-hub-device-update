#include "aducpal/unistd.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <direct.h> // rmdir
#include <io.h> // isatty, unlink, open, close
#include <sys/stat.h> // _S_*

int ADUCPAL_access(const char* pathname, int mode)
{
    if (mode != F_OK)
    {
        // TODO(JeffMill): [PAL] Only supporting F_OK (file existence)
        __debugbreak();
        return -1;
    }

    // Returns 0 on success, -1 and sets errno on failure.
    struct stat st;
    return stat(pathname, &st);
}

int ADUCPAL_chown(const char* path, uid_t owner, gid_t group)
{
    // TODO (JeffMill): [PAL] Can't really do anything here for Windows. For now, return success.
    return 0;
}

int ADUCPAL_close(int fildes)
{
    return _close(fildes);
}

gid_t ADUCPAL_getegid()
{
    __debugbreak();
    return -1;
}

uid_t ADUCPAL_geteuid()
{
    __debugbreak();
    return -1;
}

pid_t ADUCPAL_getpid()
{
    return GetCurrentProcessId();
}

uid_t ADUCPAL_getuid()
{
    __debugbreak();
    return -1;
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

ssize_t ADUCPAL_read(int fildes, void* buf, size_t nbyte)
{
    __debugbreak();
    return -1;
}

int ADUCPAL_rmdir(const char* path)
{
    return _rmdir(path);
}

int ADUCPAL_seteuid(uid_t uid)
{
    // TODO (JeffMill): [PAL] Can't really do anything here for Windows. For now, return success.
    return 0;
}

int ADUCPAL_setegid(gid_t gid)
{
    // TODO (JeffMill): [PAL] Can't really do anything here for Windows. For now, return success.
    return 0;
}

int ADUCPAL_setuid(uid_t uid)
{
    // TODO (JeffMill): [PAL] Can't really do anything here for Windows. For now, return success.
    return 0;
}

unsigned int ADUCPAL_sleep(unsigned int seconds)
{
    // TODO(JeffMill): [PAL] need to handle case of signal arriving

    // sleep() makes the calling thread sleep until
    // seconds seconds have elapsed
    // or a signal arrives which is not ignored.

    Sleep(seconds * 1000);
    return 0;
}

long ADUCPAL_syscall(long number)
{
    if (number == SYS_gettid)
    {
        return GetCurrentThreadId();
    }

    __debugbreak();
    return -1;
}

void ADUCPAL_sync()
{
    __debugbreak();
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

ssize_t ADUCPAL_write(int fildes, const void* buf, size_t nbyte)
{
    __debugbreak();
    return -1;
}
