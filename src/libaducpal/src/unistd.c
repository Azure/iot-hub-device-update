#include "aducpal/unistd.h"

#ifdef ADUCPAL_USE_PAL

#    include <io.h> // _isatty
#    include <windows.h>

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
    __debugbreak();
    return -1;
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

int ADUCPAL_gethostname(char* name, size_t len)
{
    __debugbreak();
    return -1;
}

pid_t ADUCPAL_getpid()
{
    return get_random_ulong();
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
    __debugbreak();
    return -1;
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

int rmdir(const char* path)
{
    __debugbreak();
    return -1;
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
    // Note that unlike Linux, this isn't interruptable!
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

ssize_t ADUCPAL_write(int fildes, const void* buf, size_t nbyte)
{
    __debugbreak();
    return -1;
}

#endif // #ifdef ADUCPAL_USE_PAL
