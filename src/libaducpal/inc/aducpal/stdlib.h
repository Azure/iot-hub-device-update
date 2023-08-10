#ifndef ADUCPAL_STDLIB_H
#define ADUCPAL_STDLIB_H

#ifdef ADUCPAL_USE_PAL

#    ifdef __cplusplus
extern "C"
{
#    endif

    int ADUCPAL_setenv(const char* name, const char* value, int overwrite);
    int ADUCPAL_unsetenv(const char* name);

#    ifdef __cplusplus
}
#    endif

#else

#    include <stdlib.h>

#    define ADUCPAL_setenv setenv
#    define ADUCPAL_unsetenv unsetenv

#endif // #ifdef ADUCPAL_USE_PAL

#endif // ADUCPAL_STDLIB_H
