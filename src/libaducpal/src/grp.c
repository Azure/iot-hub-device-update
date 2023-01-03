#include "aducpal/grp.h"

#ifdef ADUCPAL_USE_PAL

// Avoiding bringing in stdio.h
#    ifndef NULL
#        define NULL ((void*)0)
#    endif

struct group* ADUCPAL_getgrnam(const char* name)
{
    return NULL;
}

#endif // #ifdef ADUCPAL_USE_PAL
