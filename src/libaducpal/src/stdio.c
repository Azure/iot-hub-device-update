#include "aducpal/stdio.h"

// Avoiding bringing in stdio.h
#ifndef NULL
#    define NULL ((void*)0)
#endif

#ifdef ADUCPAL_USE_PAL

FILE* ADUCPAL_popen(const char* command, const char* type)
{
    __debugbreak();
    return NULL;
}

int ADUCPAL_pclose(FILE* stream)
{
    __debugbreak();
    return -1;
}

#endif // #ifdef ADUCPAL_USE_PAL
