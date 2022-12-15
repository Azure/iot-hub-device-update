#ifndef ADUCPAL_UNISTD_H
#define ADUCPAL_UNISTD_H

#if defined(ADUCPAL_USE_PAL)

typedef unsigned int pid_t;

#    define __NR_gettid 178
#    define SYS_gettid __NR_gettid

pid_t ADUCPAL_getpid(void);
int ADUCPAL_isatty(int fd);
unsigned int ADUCPAL_sleep(unsigned int seconds);
// Note: syscall is actually (int number, ...), but the va_arg isn't used in this project.
long ADUCPAL_syscall(long number);

#else

#    include <sys/syscall.h> // SYS_gettid
#    include <unistd.h>

#    define ADUCPAL_getpid() getpid()
#    define ADUCPAL_isatty(fd) isatty(fd)
#    define ADUCPAL_sleep(seconds) sleep(seconds)
#    define ADUCPAL_syscall(number) syscall(number)

#endif // defined(ADUCPAL_USE_PAL)

#endif // ADUCPAL_UNISTD_H
