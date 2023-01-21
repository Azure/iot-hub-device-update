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
    int ADUCPAL_open(const char* path, int oflag);
    ssize_t ADUCPAL_read(int fildes, void* buf, size_t nbyte);
    int ADUCPAL_rmdir(const char* path);
    int ADUCPAL_setegid(gid_t gid);
    int ADUCPAL_seteuid(uid_t uid);
    int ADUCPAL_setuid(uid_t uid);
    unsigned int ADUCPAL_sleep(unsigned int seconds);
    // Note: syscall is actually (int number, ...), but the va_arg isn't used in this project.
    long ADUCPAL_syscall(long number);
    void ADUCPAL_sync();
    int ADUCPAL_unlink(const char* path);

#    ifdef __cplusplus
}
#    endif

#else

#    include <sys/syscall.h> // SYS_gettid
#    include <unistd.h>

#    define ADUCPAL_access(pathname, mode) access(pathname, mode)
#    define ADUCPAL_chown(path, owner, group) chown(path, owner, group)
#    define ADUCPAL_close(fildes) close(fildes)
#    define ADUCPAL_getegid() getegid()
#    define ADUCPAL_geteuid() geteuid()
#    define ADUCPAL_getpid() getpid()
#    define ADUCPAL_getuid() getuid()
#    define ADUCPAL_isatty(fd) isatty(fd)
#    define ADUCPAL_open(path, oflag) open(path, oflag)
#    define ADUCPAL_read(fildes, buf, nbyte) read(fildes, buf, nbyte)
#    define ADUCPAL_rmdir(path) rmdir(path)
#    define ADUCPAL_setegid(gid) setegid(gid)
#    define ADUCPAL_seteuid(uid) seteuid(uid)
#    define ADUCPAL_setuid(uid) setuid(uid)
#    define ADUCPAL_sleep(seconds) sleep(seconds)
#    define ADUCPAL_syscall(number) syscall(number)
#    define ADUCPAL_sync() sync()
#    define ADUCPAL_unlink(path) unlink(path)

#endif // #ifdef ADUCPAL_USE_PAL

#endif // ADUCPAL_UNISTD_H
