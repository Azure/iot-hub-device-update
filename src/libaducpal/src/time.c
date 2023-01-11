#include "aducpal/time.h"

#include <time.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int ADUCPAL_clock_gettime(clockid_t clk_id, struct timespec* tp)
{
#define FILETIME_1970 116444736000000000ull /* seconds between 1/1/1601 and 1/1/1970 */
#define HECTONANOSEC_PER_SEC 10000000ull

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
    // TODO(JeffMill): [PAL] need to handle case of signal arriving

    // The nanosleep() function shall cause the current thread to be suspended from execution until either
    // the time interval specified by the rqtp argument has elapsed
    // or a signal is delivered to the calling thread, and its action is to invoke a signal-catching function or to terminate the process.

    // TODO(JeffMill): All callers are sending at least 1 ms.  Enforce this.
    if (rmtp != NULL || rqtp->tv_sec != 0 || rqtp->tv_nsec < 1000000)
        __debugbreak();

#define NANOSECONDS_TO_MILLISECONDS(ms) ((ms) / 1000000)

    Sleep(NANOSECONDS_TO_MILLISECONDS(rqtp->tv_nsec));

    // 0: time elapsed
    // -1: interrupted by a signal (errno set to EINTR)
    return 0;
}
