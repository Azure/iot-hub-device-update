#ifndef ADUCPAL_STRINGS_H
#define ADUCPAL_STRINGS_H

#ifdef ADUCPAL_USE_PAL

#    ifdef __cplusplus
extern "C"
{
#    endif

    int ADUCPAL_strcasecmp(const char* s1, const char* s2);

#    ifdef __cplusplus
}
#    endif

#else

#    include <strings.h>

#    define ADUCPAL_strcasecmp strcasecmp

#endif // #ifdef ADUCPAL_USE_PAL

#endif // ADUCPAL_STRINGS_H
