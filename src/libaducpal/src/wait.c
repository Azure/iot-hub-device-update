#include "aducpal/wait.h"

#ifdef ADUCPAL_USE_PAL

pid_t ADUCPAL_waitpid(pid_t pid, int* stat_loc, int options)
{
    __debugbreak();
    return -1;
}

#endif // #ifdef ADUCPAL_USE_PAL
