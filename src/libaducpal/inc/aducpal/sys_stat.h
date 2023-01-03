#ifndef ADUCPAL_SYS_STAT_H
#define ADUCPAL_SYS_STAT_H

#ifdef ADUCPAL_USE_PAL

#    include "aducpal/sys_types.h" // mode_t

#    include <dirent.h> // S_I*

// TODO(JeffMill): These aren't supported on Windows?
#    define S_ISUID 04000 /* Set user ID on execution.  */
#    define S_ISVTX 01000
#    define S_IRWXU (0400 | 0200 | 0100)
#    define S_IRWXG (S_IRWXU >> 3)

#    ifdef __cplusplus
extern "C"
{
#    endif
    int ADUCPAL_chmod(const char* path, mode_t mode);
    int ADUCPAL_fchmod(int fd, mode_t mode);
    int ADUCPAL_mkdir(const char* path, mode_t mode);
    int ADUCPAL_mkfifo(const char* pathname, mode_t mode);
#    ifdef __cplusplus
}
#    endif

#else

#    include <sys/stat.h>

#    define ADUCPAL_chmod(path, mode) chmod(path, mode)
#    define ADUCPAL_fchmod(fd, mode) fchmod(fd, mode)
#    define ADUCPAL_mkdir(path, mode) mkdir(path, mode)
#    define ADUCPAL_mkfifo(pathname, mode) mkfifo(pathname, mode)

#endif // #ifdef ADUCPAL_USE_PAL

#endif // ADUCPAL_SYS_STAT_H
