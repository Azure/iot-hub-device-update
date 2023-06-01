#ifndef ADUCPAL_UNISTD_H
#define ADUCPAL_UNISTD_H

#ifdef ADUCPAL_USE_PAL

#    include <aducpal/sys_types.h>

#    define __NR_gettid 178
#    define SYS_gettid __NR_gettid

#    define STDOUT_FILENO 1
#    define STDERR_FILENO 2

#    define F_OK 0

#    ifdef __cplusplus
extern "C"
{
#    endif

    int ADUCPAL_access(const char* pathname, int mode);
    int ADUCPAL_chown(const char* path, uid_t owner, gid_t group);
    int ADUCPAL_close(int fildes);
    gid_t ADUCPAL_getegid();
    uid_t ADUCPAL_geteuid();
    pid_t ADUCPAL_getpid();
    uid_t ADUCPAL_getuid();
    int ADUCPAL_isatty(int fd);
    int ADUCPAL_rmdir(const char* path);
    int ADUCPAL_setegid(gid_t gid);
    int ADUCPAL_seteuid(uid_t uid);
    int ADUCPAL_setuid(uid_t uid);
    // Note: syscall is actually (int number, ...), but the va_arg isn't used in this project.
    long ADUCPAL_syscall(long number);

#    ifdef __cplusplus
}
#    endif

#else

#    include <sys/syscall.h> // SYS_gettid
#    include <unistd.h>

#    define ADUCPAL_access access
#    define ADUCPAL_chown chown
#    define ADUCPAL_close close
#    define ADUCPAL_getegid getegid
#    define ADUCPAL_geteuid geteuid
#    define ADUCPAL_getpid getpid
#    define ADUCPAL_getuid getuid
#    define ADUCPAL_isatty isatty
#    define ADUCPAL_rmdir rmdir
#    define ADUCPAL_setegid setegid
#    define ADUCPAL_seteuid seteuid
#    define ADUCPAL_setuid setuid
#    define ADUCPAL_syscall syscall

#endif // #ifdef ADUCPAL_USE_PAL

#endif // ADUCPAL_UNISTD_H
