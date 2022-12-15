#include "aducpal/sys_sysinfo.h"

#ifdef ADUCPAL_USE_PAL

int ADUCPAL_sysinfo(struct sysinfo* info)
{
    __debugbreak();
    return -1;
}

#endif // #ifdef ADUCPAL_USE_PAL
