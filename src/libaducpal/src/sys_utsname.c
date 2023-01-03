#include "aducpal/sys_utsname.h"

#ifdef ADUCPAL_USE_PAL

int ADUCPAL_uname(struct utsname* buf)
{
    __debugbreak();
    return -1;
}

#endif // #ifdef ADUCPAL_USE_PAL
