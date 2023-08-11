#include "aducpal/time.h"

#include <errno.h>
#include <time.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int ADUCPAL_clock_gettime(clockid_t clk_id, struct timespec* tp)
{
#define FILETIME_1970 116444736000000000ull /* seconds between 1/1/1601 and 1/1/1970 */
#define HECTONANOSEC_PER_SEC 10000000ull

    // Note: Only CLOCK_REALTIME supported.
    if (clk_id != CLOCK_REALTIME)
    {
        _set_errno(ENOSYS);
        return -1;
    }

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

    if (gmtime_s(&tm, timep) != 0)
    {
        return NULL;
    }

    *result = tm;

    return &tm;
}

int ADUCPAL_nanosleep(const struct timespec* rqtp, struct timespec* rmtp)
{
    // Note: unlike nanosleep(), this doesn't handle signal interruption.

    // Note: rmtp argument not implemented.
    if (rmtp != NULL)
    {
        _set_errno(EINVAL);
        return -1;
    }

#define NANOSECONDS_TO_MILLISECONDS(ns) ((ns) / 1000000)
#define MILLISECONDS_TO_NANOSECONDS(ms) ((ms)*1000000)

    const unsigned long ns = rqtp->tv_nsec;
    const unsigned long ms = NANOSECONDS_TO_MILLISECONDS(ns);

    // Note: This implementation only supports ms granularity.
    if (MILLISECONDS_TO_NANOSECONDS(ms) != ns)
    {
        _set_errno(EINVAL);
        return -1;
    }

    // Add in number of seconds converted to ms.
    Sleep(ms + ((DWORD)(rqtp->tv_sec) * 1000));

    return 0;
}
