#include "aducpal/dlfcn.h"

#ifdef ADUCPAL_USE_PAL

// Avoiding bringing in stdio.h
#    ifndef NULL
#        define NULL ((void*)0)
#    endif

void* ADUCPAL_dlopen(const char* filename, int flag)
{
    __debugbreak();
    return NULL;
}

char* ADUCPAL_dlerror(void)
{
    __debugbreak();
    return "NYI";
}

void* ADUCPAL_dlsym(void* handle, const char* symbol)
{
    __debugbreak();
    return NULL;
}

int ADUCPAL_dlclose(void* handle)
{
    __debugbreak();
    return 0;
}

#endif // #ifdef ADUCPAL_USE_PAL
