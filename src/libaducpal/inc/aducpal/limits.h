#ifndef ADUC_AL_LIMITS_H
#define ADUC_AL_LIMITS_H

#ifdef ADUCPAL_USE_PAL

#    ifdef __cplusplus
extern "C"
{
#    endif

#    define PATH_MAX 4096 // Maximum size of a filepath
#    ifdef __cplusplus
}
#    endif

#else

#    include <limits.h>

#endif // #ifdef ADUCPAL_USE_PAL

#endif // ADUCPAL_GRP_H
