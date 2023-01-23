#ifndef ADUCPAL_STDLIB_H
#define ADUCPAL_STDLIB_H

#ifdef ADUCPAL_USE_PAL

#    ifdef __cplusplus
extern "C"
{
#    endif

    char* ADUCPAL_mktemp(char* tmpl);
    int ADUCPAL_setenv(const char* name, const char* value, int overwrite);
    int ADUCPAL_unsetenv(const char* name);

#    ifdef __cplusplus
}
#    endif

#else

#    include <stdlib.h>

#    define ADUCPAL_mktemp(tmpl) mktemp(tmpl)
#    define ADUCPAL_setenv(name, value, overwrite) setenv(name, value, overwrite)
#    define ADUCPAL_unsetenv(name) unsetenv(name)

#endif // #ifdef ADUCPAL_USE_PAL

#endif // ADUCPAL_STDLIB_H
