#ifndef ADUCPAL_TIME_H
#define ADUCPAL_TIME_H

#ifdef ADUCPAL_USE_PAL

#    include <time.h>

typedef unsigned int clockid_t;

#    define CLOCK_REALTIME 0

#    ifdef __cplusplus
extern "C"
{
#    endif
    int ADUCPAL_clock_gettime(clockid_t clockid, struct timespec* tp);
    struct tm* ADUCPAL_gmtime_r(const time_t* timep, struct tm* result);
    int ADUCPAL_nanosleep(const struct timespec* rqtp, struct timespec* rmtp);
#    ifdef __cplusplus
}
#    endif

#else

#    include <time.h>

#    define ADUCPAL_clock_gettime(clockid, tp) clock_gettime(clockid, tp)
#    define ADUCPAL_gmtime_r(timep, result) gmtime_r(timep, result)
#    define ADUCPAL_nanosleep(rqtp, rmtp) nanosleep(rqtp, rmtp)

#endif // #ifdef ADUCPAL_USE_PAL

#endif // ADUCPAL_TIME_H
