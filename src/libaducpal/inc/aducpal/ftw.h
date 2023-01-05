#ifndef ADUCPAL_FTW_H
#define ADUCPAL_FTW_H

#ifdef ADUCPAL_USE_PAL

typedef struct _FTW
{
    void* unused;
} FTW;

enum
{
    FTW_DEPTH = 1,
    FTW_PHYS = 2,
    FTW_MOUNT = 4,
    FTW_CHDIR = 8
} FTW_FLAGS;

enum
{
    FTW_F = 1,
    FTW_D = 2,
    FTW_DNR = 3,
    FTW_DP = 4,
    FTW_NS = 5,
    FTW_SL = 6,
    FTW_SLN = 7
} FTW_TYPE;

#    ifdef __cplusplus
extern "C"
{
#    endif

    int ADUCPAL_nftw(
        const char* dirpath,
        int (*fn)(const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf),
        int nopenfd,
        int flags);

#    ifdef __cplusplus
}
#    endif

#else

#    define __USE_XOPEN_EXTENDED 1
#    include <ftw.h>

#    define ADUCPAL_nftw(dirpath, fn, nopenfd, flags) nftw(dirpath, fn, nopenfd, flags)

#endif // #ifdef ADUCPAL_USE_PAL

#endif // ADUCPAL_FTW_H
