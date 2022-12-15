#ifndef ADUCPAL_SYS_TIME_H
#define ADUCPAL_SYS_TIME_H

#if defined(ADUCPAL_USE_PAL)

#    include <time.h> // time_t

struct timeval
{
    time_t tv_sec;
    // tv_nsec not referenced in this project.
};

// Note: unclear if tz is "struct timezone*" or "void*".
// In any case, it's unused in this project.
int ADUCPAL_gettimeofday(struct timeval* tv, void* tz);

#else

#    include <sys/time.h>

#    define ADUCPAL_gettimeofday(tv, tz) gettimeofday(tv, tz)

#endif // defined(ADUCPAL_USE_PAL)

#endif // ADUCPAL_SYS_TIME_H
