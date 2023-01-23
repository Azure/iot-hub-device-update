#include "aducpal/signal.h"
#include <errno.h>

#ifndef UNREFERENCED_PARAMETER
#    define UNREFERENCED_PARAMETER(param) (void)(param)
#endif

int ADUCPAL_raise(int sig)
{
    // TODO(JeffMill): [PAL] NYI
    UNREFERENCED_PARAMETER(sig);
    _set_errno(ENOSYS);
    return -1;
}
