#include "aducpal/ftw.h"

#ifdef ADUCPAL_USE_PAL

int ADUCPAL_nftw(
    const char* dirpath,
    int (*fn)(const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf),
    int nopenfd,
    int flags)
{
    __debugbreak();
    return -1; // failure
}

#endif // #ifdef ADUCPAL_USE_PAL
