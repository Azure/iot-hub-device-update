#include "aducpal/unistd.h"

#ifdef ADUCPAL_USE_PAL

#    include <io.h> // _isatty
#    include <windows.h>

static unsigned long get_random_ulong()
{
    static unsigned long next = 1;

    // From https://pubs.opengroup.org/onlinepubs/009695399/functions/rand.html
    next = next * 1103515245 + 12345;
    return ((unsigned)(next / 65536) % 32768);
}

// TODO(JeffMill): Implement
pid_t ADUCPAL_getpid()
{
    return get_random_ulong();
}

// TODO(JeffMill): Implement
int ADUCPAL_isatty(int fd)
{
    return _isatty(fd);
}

// TODO(JeffMill): Implement
unsigned int ADUCPAL_sleep(unsigned int seconds)
{
    // Note that unlike Linux, this isn't interruptable!
    Sleep(seconds * 1000);
    return 0;
}

// TODO(JeffMill): Implement
long ADUCPAL_syscall(long number)
{
    if (number != SYS_gettid)
    {
        __debugbreak();
    }

    return get_random_ulong();
}

#endif // #ifdef ADUCPAL_USE_PAL
