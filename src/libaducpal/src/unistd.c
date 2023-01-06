#include "aducpal/unistd.h"

#ifdef ADUCPAL_USE_PAL

// clang-format off
#define WIN32_LEAN_AND_MEAN
#include <windows.h> // GetCurrentProcessId
#include <sys/stat.h> // _S_*
#include <direct.h> // rmdir
#include <io.h> // isatty, unlink, open, close
// clang-format on

static unsigned long get_random_ulong()
{
    static unsigned long next = 1;

    // From https://pubs.opengroup.org/onlinepubs/009695399/functions/rand.html
    next = next * 1103515245 + 12345;
    return ((unsigned)(next / 65536) % 32768);
}

int ADUCPAL_access(const char* pathname, int mode)
{
    __debugbreak();
    return -1;
}

int ADUCPAL_chown(const char* path, uid_t owner, gid_t group)
{
    __debugbreak();
    return -1;
}

int ADUCPAL_close(int fildes)
{
    return _close(fildes);
}

int ADUCPAL_dup2(int fildes, int fildes2)
{
    __debugbreak();
    return -1;
}

int ADUCPAL_execvp(const char* file, char* const argv[])
{
    __debugbreak();
    errno = ENOSYS;
    return -1;
}

pid_t ADUCPAL_fork()
{
    __debugbreak();
    return -1;
}

gid_t ADUCPAL_getegid(void)
{
    return get_random_ulong();
}

uid_t ADUCPAL_geteuid(void)
{
    return get_random_ulong();
}

pid_t ADUCPAL_getpid()
{
    return GetCurrentProcessId();
}

uid_t ADUCPAL_getuid()
{
    return get_random_ulong();
}

int ADUCPAL_isatty(int fd)
{
    return _isatty(fd);
}

int ADUCPAL_open(const char* path, int oflag)
{
    // return _open(path, oflag);
    int fd;
    int ret = _sopen_s(&fd, path, oflag, _SH_DENYNO, _S_IREAD | _S_IWRITE);
    if (ret != 0)
    {
        return -1;
    }
    return fd;
}

int ADUCPAL_pipe(int fildes[2])
{
    __debugbreak();
    return -1;
}

ssize_t ADUCPAL_read(int fildes, void* buf, size_t nbyte)
{
    __debugbreak();
    return -1;
}

int ADUCPAL_rmdir(const char* path)
{
    __debugbreak();
    return _rmdir(path);
}

int ADUCPAL_seteuid(uid_t uid)
{
    __debugbreak();
    return 0;
}

int ADUCPAL_setegid(gid_t gid)
{
    __debugbreak();
    return 0;
}

int ADUCPAL_setuid(uid_t uid)
{
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
    if (number != SYS_gettid)
    {
        __debugbreak();
    }

    return get_random_ulong();
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

#endif // #ifdef ADUCPAL_USE_PAL
