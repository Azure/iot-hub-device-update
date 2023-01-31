#ifndef ADUCPAL_SYS_TIME_H
#define ADUCPAL_SYS_TIME_H

#ifdef ADUCPAL_USE_PAL

#    include <aducpal/time.h> // time_t

// This project only references tv_sec
struct timeval
{
    time_t tv_sec; /* seconds */
    // suseconds_t tv_usec;    /* microseconds */
};

#    ifdef __cplusplus
extern "C"
{
#    endif

    // Note: unclear if tz is "struct timezone*" or "void*".
    // In any case, it's unused in this project.
    int ADUCPAL_gettimeofday(struct timeval* tv, void* tz);

#    ifdef __cplusplus
}
#    endif

#else

#    include <sys/time.h>

#    define ADUCPAL_gettimeofday gettimeofday

#endif // #ifdef ADUCPAL_USE_PAL

#endif // ADUCPAL_SYS_TIME_H
