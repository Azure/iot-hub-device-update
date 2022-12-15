#ifndef ADUCPAL_SIGNAL_H
#define ADUCPAL_SIGNAL_H

#ifdef ADUCPAL_USE_PAL

#    define SIGUSR1 10

#    ifdef __cplusplus
extern "C"
{
#    endif
    int ADUCPAL_raise(int sig);

#    ifdef __cplusplus
}
#    endif

#else

#    include <signal.h>

#    define ADUCPAL_raise(sig) raise(sig)

#endif // #ifdef ADUCPAL_USE_PAL

#endif // ADUCPAL_SIGNAL_H
