#ifndef ADUC_PAL_LIMITS_H
#define ADUC_PAL_LIMITS_H

#ifdef ADUCPAL_USE_PAL
#    ifndef MAX_PATH
#        define MAX_PATH 260
#    endif

#    include <limits.h>

#    ifndef PATH_MAX
#        define PATH_MAX MAX_PATH
#    endif
#else
#    include <limits.h>

#    ifndef MAX_PATH

#        define MAX_PATH PATH_MAX

#    endif
#endif
#endif // ADUCPAL_GRP_H
