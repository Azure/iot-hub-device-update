#include "aducpal/time.h"

#ifdef ADUCPAL_USE_PAL

#    include <time.h>
#    include <windows.h>

int ADUCPAL_clock_gettime(clockid_t clk_id, struct timespec* tp)
{
#    define FILETIME_1970 116444736000000000ull /* seconds between 1/1/1601 and 1/1/1970 */
#    define HECTONANOSEC_PER_SEC 10000000ull

    ULARGE_INTEGER fti;
    GetSystemTimeAsFileTime((FILETIME*)&fti);
    fti.QuadPart -= FILETIME_1970; /* 100 nano-seconds since 1-1-1970 */
    tp->tv_sec = fti.QuadPart / HECTONANOSEC_PER_SEC; /* seconds since 1-1-1970 */
    tp->tv_nsec = (long)(fti.QuadPart % HECTONANOSEC_PER_SEC) * 100; /* nanoseconds */
    return 0;
}

struct tm* ADUCPAL_gmtime_r(const time_t* timep, struct tm* result)
{
    static struct tm tm;
    const struct tm* gmtime = _gmtime64(timep);
    if (gmtime == NULL)
    {
        __debugbreak();
        return NULL;
    }

    memcpy(&tm, gmtime, sizeof(tm));

    *result = tm;
    return &tm;
}

int ADUCPAL_nanosleep(const struct timespec* rqtp, struct timespec* rmtp)
{
    __debugbreak();
    return -1;
}

#endif // #ifdef ADUCPAL_USE_PAL
