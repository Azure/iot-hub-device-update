#ifndef ADUCPAL_TIME_H
#define ADUCPAL_TIME_H

#if defined(ADUCPAL_USE_PAL)

#    include <time.h> // timespec

typedef unsigned int clockid_t;

#    define CLOCK_REALTIME 0

int ADUCPAL_clock_gettime(clockid_t clockid, struct timespec* tp);

struct tm* ADUCPAL_gmtime_r(const time_t* timep, struct tm* result);

#else

#    include <time.h>

#    define ADUCPAL_clock_gettime(clockid, tp) clock_gettime(clockid, tp)

#    define ADUCPAL_gmtime_r(timep, result) gmtime_r(timep, result)

#endif // defined(ADUCPAL_USE_PAL)

#endif // ADUCPAL_TIME_H
