#include "aducpal/signal.h"

#ifdef ADUCPAL_USE_PAL

int ADUCPAL_raise(int sig)
{
    __debugbreak();
    return -1;
}

#endif // #ifdef ADUCPAL_USE_PAL
