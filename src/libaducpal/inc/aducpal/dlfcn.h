#ifndef ADUCPAL_DLFCN_H
#define ADUCPAL_DLFCN_H

#ifdef ADUCPAL_USE_PAL

#    define RTLD_LAZY 0

#    ifdef __cplusplus
extern "C"
{
#    endif

    void* ADUCPAL_dlopen(const char* filename, int flag);
    char* ADUCPAL_dlerror(void);
    void* ADUCPAL_dlsym(void* handle, const char* symbol);
    int ADUCPAL_dlclose(void* handle);

#    ifdef __cplusplus
}
#    endif

#else

#    include <dlfcn.h>

#    define ADUCPAL_dlopen(filename, flag) dlopen(filename, flag)
#    define ADUCPAL_dlerror() dlerror()
#    define ADUCPAL_dlsym(handle, symbol) dlsym(handle, symbol)
#    define ADUCPAL_dlclose(handle) dlclose(handle)

#endif // #ifdef ADUCPAL_USE_PAL

#endif // ADUCPAL_DLFCN_H
