#include "aducpal/sys_statvfs.h"

#ifdef ADUCPAL_USE_PAL

int ADUCPAL_statvfs(const char* path, struct statvfs* buf)
{
    __debugbreak();
    return -1;
}

#endif // #ifdef ADUCPAL_USE_PAL
