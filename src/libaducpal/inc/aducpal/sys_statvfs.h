#ifndef ADUCPAL_SYS_STATVFS_H
#define ADUCPAL_SYS_STATVFS_H

#ifdef ADUCPAL_USE_PAL

typedef unsigned long fsblkcnt_t;

// This project only references f_frsize, f_blocks
struct statvfs
{
    // unsigned long f_bsize; /* Filesystem block size */
    unsigned long f_frsize; /* Fragment size */
    fsblkcnt_t f_blocks; /* Size of fs in f_frsize units */
    // fsblkcnt_t f_bfree; /* Number of free blocks */
    // fsblkcnt_t f_bavail; /* Number of free blocks for unprivileged users */
    // fsfilcnt_t f_files; /* Number of inodes */
    // fsfilcnt_t f_ffree; /* Number of free inodes */
    // fsfilcnt_t f_favail; /* Number of free inodes for unprivileged users */
    // unsigned long f_fsid; /* Filesystem ID */
    // unsigned long f_flag; /* Mount flags */
    // unsigned long f_namemax; /* Maximum filename length */
};

#    ifdef __cplusplus
extern "C"
{
#    endif

    int ADUCPAL_statvfs(const char* path, struct statvfs* buf);

#    ifdef __cplusplus
}
#    endif

#else

#    include <sys/statvfs.h>

#    define ADUCPAL_statvfs statvfs

#endif // #ifdef ADUCPAL_USE_PAL

#endif // ADUCPAL_SYS_STATVFS_H
