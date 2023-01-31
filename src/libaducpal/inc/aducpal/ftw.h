#ifndef ADUCPAL_FTW_H
#define ADUCPAL_FTW_H

#ifdef ADUCPAL_USE_PAL

struct stat;

/* Values for the FLAG argument to the user function passed to `ftw'
   and 'nftw'.  */
enum
{
    FTW_F, /* Regular file.  */
#    define FTW_F FTW_F
    FTW_D, /* Directory.  */
#    define FTW_D FTW_D
    FTW_DNR, /* Unreadable directory.  */
#    define FTW_DNR FTW_DNR
    FTW_NS, /* Unstatable file.  */
#    define FTW_NS FTW_NS
    FTW_SL, /* Symbolic link.  */
#    define FTW_SL FTW_SL
    /* These flags are only passed from the `nftw' function.  */
    FTW_DP, /* Directory, all subdirs have been visited. */
#    define FTW_DP FTW_DP
    FTW_SLN /* Symbolic link naming non-existing file.  */
};

/* Flags for fourth argument of `nftw'.  */
enum
{
    FTW_PHYS = 1, /* Perform physical walk, ignore symlinks.  */
#    define FTW_PHYS FTW_PHYS
    FTW_MOUNT = 2, /* Report only files on same file system as the
			   argument.  */
#    define FTW_MOUNT FTW_MOUNT
    FTW_CHDIR = 4, /* Change to current directory while processing it.  */
#    define FTW_CHDIR FTW_CHDIR
    FTW_DEPTH = 8 /* Report files in directory before directory itself.*/
#    define FTW_DEPTH FTW_DEPTH
};

/* Structure used for fourth argument to callback function for `nftw'.  */
struct FTW
{
    int base;
    int level;
};

#    ifdef __cplusplus
extern "C"
{
#    endif

    typedef int (*NFTW_FUNC_T)(const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf);

    int ADUCPAL_nftw(const char* dirpath, NFTW_FUNC_T fn, int nopenfd, int flags);

#    ifdef __cplusplus
}
#    endif

#else

#    define __USE_XOPEN_EXTENDED 1
#    include <ftw.h>

#    define ADUCPAL_nftw nftw

#endif // #ifdef ADUCPAL_USE_PAL

#endif // ADUCPAL_FTW_H
