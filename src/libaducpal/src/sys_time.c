#include "aducpal/sys_time.h"
#include "aducpal/time.h" // clock_gettime

#include <errno.h>
#include <string.h> // memset

int ADUCPAL_gettimeofday(struct timeval* tv, void* tzp)
{
    memset(tv, 0, sizeof(*tv));

    // If tzp is not a null pointer, the behavior is unspecified.
    if (tzp != NULL)
    {
        // No errors are defined.
        return ENOSYS;
    }

    struct timespec ts;
    ADUCPAL_clock_gettime(CLOCK_REALTIME, &ts);

    tv->tv_sec = ts.tv_sec;
    // Not used in this project.
    // tv->tv_usec = ts.tv_nsec / 1000;
    return 0;
}
